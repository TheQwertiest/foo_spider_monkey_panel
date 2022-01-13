#include <stdafx.h>

#include "js_container.h"

#include <js_engine/js_engine.h>
#include <js_engine/js_gc.h>
#include <js_engine/js_realm_inner.h>
#include <js_objects/drop_source_action.h>
#include <js_objects/gdi_graphics.h>
#include <js_objects/global_object.h>
#include <js_utils/js_async_task.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/scope_helper.h>
#include <panel/js_panel_window.h>

#include <qwr/final_action.h>

SMP_MJS_SUPPRESS_WARNINGS_PUSH
#include <js/CompilationAndEvaluation.h>
#include <js/SourceText.h>
#include <js/Wrapper.h>
SMP_MJS_SUPPRESS_WARNINGS_POP

using namespace smp;

namespace mozjs
{

JsContainer::JsContainer( panel::js_panel_window& parentPanel )
{
    pParentPanel_ = &parentPanel;

    bool bRet = JsEngine::GetInstance().RegisterContainer( *this );
    jsStatus_ = ( bRet ? JsStatus::Ready : JsStatus::EngineFailed );
}

JsContainer::~JsContainer()
{
    Finalize();
    JsEngine::GetInstance().UnregisterContainer( *this );
    pJsCtx_ = nullptr;
}

bool JsContainer::Initialize()
{
    if ( JsStatus::EngineFailed == jsStatus_ )
    {
        Fail( "JS engine failed to initialize" );
        return false;
    }

    assert( pJsCtx_ );
    assert( pParentPanel_ );

    if ( JsStatus::Working == jsStatus_ )
    {
        return true;
    }

    if ( jsGlobal_.initialized() || jsGraphics_.initialized() )
    {
        jsGraphics_.reset();
        jsGlobal_.reset();
    }

    try
    {
        jsGlobal_.init( pJsCtx_, JsGlobalObject::CreateNative( pJsCtx_, *this ) );
        assert( jsGlobal_ );
        qwr::final_action autoGlobal( [&jsGlobal = jsGlobal_] {
            jsGlobal.reset();
        } );

        JSAutoRealm ac( pJsCtx_, jsGlobal_ );

        jsGraphics_.init( pJsCtx_, JsGdiGraphics::CreateJs( pJsCtx_ ) );

        pNativeRealm_ = static_cast<JsRealmInner*>( JS::GetRealmPrivate( js::GetContextRealm( pJsCtx_ ) ) );
        assert( pNativeRealm_ );

        autoGlobal.cancel();
    }
    catch ( ... )
    {
        Fail( mozjs::error::ExceptionToText( pJsCtx_ ) );
        return false;
    }

    pNativeGlobal_ = static_cast<JsGlobalObject*>( JS_GetPrivate( jsGlobal_ ) );
    assert( pNativeGlobal_ );
    pNativeGraphics_ = static_cast<JsGdiGraphics*>( JS_GetPrivate( jsGraphics_ ) );
    assert( pNativeGraphics_ );

    jsStatus_ = JsStatus::Working;

    return true;
}

void JsContainer::Finalize()
{
    if ( JsStatus::Ready == jsStatus_ )
    {
        return;
    }

    if ( JsStatus::Failed != jsStatus_ && JsStatus::EngineFailed != jsStatus_ )
    { // Don't suppress error: it should be cleared only on initialization
        jsStatus_ = JsStatus::Ready;
    }

    pNativeGraphics_ = nullptr;
    jsGraphics_.reset();
    jsDropAction_.reset();
    if ( !jsGlobal_.initialized() )
    {
        return;
    }

    {
        JSAutoRealm ac( pJsCtx_, jsGlobal_ );

        JsGlobalObject::PrepareForGc( pJsCtx_, jsGlobal_ );

        auto pJsRealm = static_cast<JsRealmInner*>( JS::GetRealmPrivate( js::GetContextRealm( pJsCtx_ ) ) );
        assert( pJsRealm );

        pNativeRealm_ = nullptr;
        pJsRealm->MarkForDeletion();
    }

    pNativeGlobal_ = nullptr;
    jsGlobal_.reset();

    (void)JsEngine::GetInstance().GetGcEngine().TriggerGc();
}

void JsContainer::Fail( const qwr::u8string& errorText )
{
    Finalize();
    if ( JsStatus::EngineFailed != jsStatus_ )
    { // Don't suppress error
        jsStatus_ = JsStatus::Failed;
    }

    assert( pParentPanel_ );
    const qwr::u8string errorTextPadded = [pParentPanel = pParentPanel_, &errorText]() {
        qwr::u8string text =
            fmt::format( "Error: " SMP_NAME_WITH_VERSION " ({})", pParentPanel->GetPanelDescription() );
        if ( !errorText.empty() )
        {
            text += "\n";
            text += errorText;
        }

        return text;
    }();

    pParentPanel_->Fail( errorTextPadded );
}

JsContainer::JsStatus JsContainer::GetStatus() const
{
    return jsStatus_;
}

bool JsContainer::ExecuteScript( const qwr::u8string& scriptCode )
{
    assert( pJsCtx_ );
    assert( jsGlobal_.initialized() );
    assert( JsStatus::Working == jsStatus_ );

    auto selfSaver = shared_from_this();
    isParsingScript_ = true;
    const auto autoParseState = qwr::final_action( [&] { isParsingScript_ = false; } );

    JSAutoRealm ac( pJsCtx_, jsGlobal_ );
    try
    {
        JS::SourceText<mozilla::Utf8Unit> source;
        if ( !source.init( pJsCtx_, scriptCode.c_str(), scriptCode.length(), JS::SourceOwnership::Borrowed ) )
        {
            throw JsException();
        }

        JS::CompileOptions opts( pJsCtx_ );
        opts.setFileAndLine( "", 1 );

        OnJsActionStart();
        qwr::final_action autoAction( [&] { OnJsActionEnd(); } );

        JS::RootedValue dummyRval( pJsCtx_ );
        if ( !JS::Evaluate( pJsCtx_, opts, source, &dummyRval ) )
        {
            throw JsException();
        }
        return true;
    }
    catch ( ... )
    {
        mozjs::error::ExceptionToJsError( pJsCtx_ );
        Fail( mozjs::error::JsErrorToText( pJsCtx_ ) );
        return false;
    }
}

bool JsContainer::ExecuteScriptFile( const std::filesystem::path& scriptPath )
{
    assert( pJsCtx_ );
    assert( jsGlobal_.initialized() );
    assert( JsStatus::Working == jsStatus_ );

    auto selfSaver = shared_from_this();
    isParsingScript_ = true;
    auto autoParseState = qwr::final_action( [&] { isParsingScript_ = false; } );

    JSAutoRealm ac( pJsCtx_, jsGlobal_ );
    try
    {
        OnJsActionStart();
        qwr::final_action autoAction( [&] { OnJsActionEnd(); } );

        assert( pNativeGlobal_ );
        pNativeGlobal_->IncludeScript( scriptPath.u8string() );
        return true;
    }
    catch ( ... )
    {
        mozjs::error::ExceptionToJsError( pJsCtx_ );
        Fail( mozjs::error::JsErrorToText( pJsCtx_ ) );
        return false;
    }
}

void JsContainer::RunJobs()
{
    JsEngine::GetInstance().MaybeRunJobs();
}

smp::panel::js_panel_window& JsContainer::GetParentPanel() const
{
    assert( pParentPanel_ );
    return *pParentPanel_;
}

bool JsContainer::InvokeOnDragAction( const qwr::u8string& functionName, const POINTL& pt, uint32_t keyState, panel::DragActionParams& actionParams )
{
    if ( !IsReadyForCallback() )
    {
        return false;
    }

    auto selfSaver = shared_from_this();
    JsAutoRealmWithErrorReport autoScope( pJsCtx_, jsGlobal_ );

    if ( !CreateDropActionIfNeeded() )
    { // reports
        return false;
    }

    pNativeDropAction_->AccessDropActionParams() = actionParams;

    auto retVal = InvokeJsCallback( functionName,
                                    static_cast<JS::HandleObject>( jsDropAction_ ),
                                    static_cast<int32_t>( pt.x ),
                                    static_cast<int32_t>( pt.y ),
                                    static_cast<uint32_t>( keyState ) );
    if ( !retVal )
    {
        return false;
    }

    actionParams = pNativeDropAction_->AccessDropActionParams();
    return true;
}

void JsContainer::InvokeOnNotify( const std::wstring& name, JS::HandleValue info )
{
    if ( !IsReadyForCallback() )
    {
        return;
    }

    auto selfSaver = shared_from_this();
    JsAutoRealmWithErrorReport autoScope( pJsCtx_, jsGlobal_ );

    // Bind object to current realm
    JS::RootedValue jsValue( pJsCtx_, info );
    if ( !JS_WrapValue( pJsCtx_, &jsValue ) )
    { // reports
        return;
    }

    autoScope.DisableReport(); ///< InvokeJsCallback has it's own AutoReportException
    (void)InvokeJsCallback( "on_notify_data",
                            name,
                            static_cast<JS::HandleValue>( jsValue ) );
    if ( jsValue.isObject() )
    { // this will remove all wrappers (e.g. during callback re-entrancy)
        js::NukeCrossCompartmentWrappers( pJsCtx_,
                                          js::SingleCompartment{ js::GetContextCompartment( pJsCtx_ ) },
                                          js::GetNonCCWObjectRealm( js::UncheckedUnwrap( &jsValue.toObject() ) ),
                                          js::NukeReferencesToWindow::DontNukeWindowReferences, ///< browser specific flag, irrelevant to us
                                          js::NukeReferencesFromTarget::NukeIncomingReferences );
    }
}

void JsContainer::InvokeOnPaint( Gdiplus::Graphics& gr )
{
    if ( !IsReadyForCallback() )
    {
        return;
    }

    auto selfSaver = shared_from_this();
    pNativeGraphics_->SetGraphicsObject( &gr );

    (void)InvokeJsCallback( "on_paint",
                            static_cast<JS::HandleObject>( jsGraphics_ ) );
    if ( pNativeGraphics_ )
    { // InvokeJsCallback invokes Fail() on error, which resets pNativeGraphics_
        pNativeGraphics_->SetGraphicsObject( nullptr );
    }
}

bool JsContainer::InvokeJsAsyncTask( JsAsyncTask& jsTask )
{
    if ( !IsReadyForCallback() )
    {
        return true;
    }

    auto selfSaver = shared_from_this();
    JsAutoRealmWithErrorReport autoScope( pJsCtx_, jsGlobal_ );

    OnJsActionStart();
    qwr::final_action autoAction( [&] { OnJsActionEnd(); } );

    return jsTask.InvokeJs();
}

void JsContainer::SetJsCtx( JSContext* cx )
{
    assert( cx );
    pJsCtx_ = cx;
}

bool JsContainer::IsReadyForCallback() const
{
    return ( JsStatus::Working == jsStatus_ ) && !isParsingScript_;
}

bool JsContainer::CreateDropActionIfNeeded()
{
    if ( jsDropAction_.initialized() )
    {
        return true;
    }

    try
    {
        jsDropAction_.init( pJsCtx_, JsDropSourceAction::CreateJs( pJsCtx_ ) );
    }
    catch ( ... )
    {
        mozjs::error::ExceptionToJsError( pJsCtx_ );
        return false;
    }

    pNativeDropAction_ = static_cast<JsDropSourceAction*>( JS_GetPrivate( jsDropAction_ ) );

    return true;
}

void JsContainer::OnJsActionStart()
{
    if ( nestedJsCounter_++ == 0 )
    {
        JsEngine::GetInstance().OnJsActionStart( *this );
    }
}

void JsContainer::OnJsActionEnd()
{
    if ( --nestedJsCounter_ == 0 )
    {
        JsEngine::GetInstance().OnJsActionEnd( *this );
    }
}

} // namespace mozjs
