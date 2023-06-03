#include <stdafx.h>

#include "js_container.h"

#include <panel/panel_window.h>

SMP_MJS_SUPPRESS_WARNINGS_PUSH
#include <js/Wrapper.h>
SMP_MJS_SUPPRESS_WARNINGS_POP
#include <js_backend/engine/js_engine.h>
#include <js_backend/engine/js_gc.h>
#include <js_backend/engine/js_realm_inner.h>
#include <js_backend/objects/core/global_object.h>
#include <js_backend/objects/dom/drop_source_action.h>
#include <js_backend/objects/dom/event_target.h>
#include <js_backend/objects/gdi/gdi_graphics.h>
#include <js_backend/utils/js_async_task.h>
#include <js_backend/utils/js_error_helper.h>
#include <js_backend/utils/scope_helper.h>
#include <tasks/events/js_target_event.h>

#include <qwr/final_action.h>

using namespace smp;

namespace mozjs
{

JsContainer::JsContainer( panel::PanelWindow& parentPanel )
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
        qwr::final_action autoGlobal( [&jsGlobal = jsGlobal_] { jsGlobal.reset(); } );

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

    pNativeGlobal_ = static_cast<JsGlobalObject*>( mozjs::utils::GetMaybePtrFromReservedSlot( jsGlobal_, kReservedObjectSlot ) );
    assert( pNativeGlobal_ );
    pNativeGraphics_ = JsGdiGraphics::ExtractNativeUnchecked( jsGraphics_ );
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

bool JsContainer::ExecuteScript( const qwr::u8string& scriptCode, bool isModule )
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
        OnJsActionStart();
        qwr::final_action autoAction( [&] { OnJsActionEnd(); } );

        assert( pNativeGlobal_ );
        // TODO: fix
        pNativeGlobal_->GetScriptLoader().ExecuteTopLevelScript( scriptCode, true );

        return true;
    }
    catch ( ... )
    {
        mozjs::error::ExceptionToJsError( pJsCtx_ );
        Fail( mozjs::error::JsErrorToText( pJsCtx_ ) );
        return false;
    }
}

bool JsContainer::ExecuteScriptFile( const std::filesystem::path& scriptPath, bool isModule )
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
        pNativeGlobal_->GetScriptLoader().ExecuteTopLevelScriptFile( scriptPath, isModule );
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

smp::panel::PanelWindow& JsContainer::GetParentPanel() const
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

EventStatus JsContainer::InvokeJsEvent( smp::EventBase& event )
{
    if ( !IsReadyForCallback() )
    {
        return EventStatus{};
    }

    auto selfSaver = shared_from_this();
    JsAutoRealmWithErrorReport autoScope( pJsCtx_, jsGlobal_ );

    OnJsActionStart();
    qwr::final_action autoAction( [&] { OnJsActionEnd(); } );

    try
    {
        if ( event.GetId() == smp::EventId::kNew_JsTarget )
        {
            auto& targetEvent = static_cast<smp::JsTargetEvent&>( event );
            JS::RootedObject jsTarget( pJsCtx_, targetEvent.GetJsTarget() );

            auto pNativeTarget = JsEventTarget::ExtractNative( pJsCtx_, jsTarget );
            assert( pNativeTarget );

            return pNativeTarget->HandleEvent( jsTarget, event );
        }
        else
        {
            return pNativeGlobal_->HandleEvent( event );
        }
    }
    catch ( ... )
    {
        mozjs::error::ExceptionToJsError( pJsCtx_ );
        return EventStatus{};
    }
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

    pNativeDropAction_ = JsDropSourceAction::ExtractNativeUnchecked( jsDropAction_ );

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
