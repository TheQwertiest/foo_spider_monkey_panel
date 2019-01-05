#include <stdafx.h>
#include "js_container.h"

#include <js_engine/js_engine.h>
#include <js_engine/js_compartment_inner.h>
#include <js_objects/global_object.h>
#include <js_objects/gdi_graphics.h>
#include <js_objects/drop_source_action.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/scope_helper.h>
#include <js_utils/js_async_task.h>
#include <utils/scope_helpers.h>

#include <js_panel_window.h>
#include <host_timer_dispatcher.h>
#include <smp_exception.h>

#pragma warning( push )
#pragma warning( disable : 4100 ) // unused variable
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <js/Wrapper.h>
#pragma warning( pop )

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

void JsContainer::SetJsCtx( JSContext* cx )
{
    assert( cx );
    pJsCtx_ = cx;
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
        JSAutoRequest ar( pJsCtx_ );

        jsGlobal_.init( pJsCtx_, JsGlobalObject::CreateNative( pJsCtx_, *this, *pParentPanel_ ) );
        assert( jsGlobal_ );
        utils::final_action autoGlobal( [&]() {
            jsGlobal_.reset();
        } );

        JSAutoCompartment ac( pJsCtx_, jsGlobal_ );

        jsGraphics_.init( pJsCtx_, JsGdiGraphics::CreateJs( pJsCtx_ ) );

        autoGlobal.cancel();
    }
    catch ( ... )
    {
        mozjs::error::ExceptionToJsError( pJsCtx_ );
        Fail( mozjs::error::JsErrorToText( pJsCtx_ ) );
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

    HostTimerDispatcher::Get().onPanelUnload( pParentPanel_->GetHWND() );

    {
        JSAutoRequest ar( pJsCtx_ );
        JSAutoCompartment ac( pJsCtx_, jsGlobal_ );

        JsGlobalObject::PrepareForGc( pJsCtx_, jsGlobal_ );

        auto pJsCompartment = static_cast<JsCompartmentInner*>( JS_GetCompartmentPrivate( js::GetContextCompartment( pJsCtx_ ) ) );
        assert( pJsCompartment );

        pJsCompartment->MarkForDeletion();
    }

    jsGlobal_.reset();
}

void JsContainer::Fail( const pfc::string8_fast& errorText )
{
    Finalize();
    if ( JsStatus::EngineFailed != jsStatus_ )
    { // Don't supress error
        jsStatus_ = JsStatus::Failed;
    }

    assert( pParentPanel_ );
    const pfc::string8_fast errorTextPadded = [pParentPanel = pParentPanel_, &errorText]() {
        pfc::string8_fast text = "Error: " SMP_NAME_WITH_VERSION;
        text += " (";
        text += pParentPanel->ScriptInfo().build_info_string();
        text += ")";
        if ( !errorText.is_empty() )
        {
            text += "\n";
            text += errorText;
        }

        return text;
    }();

    FB2K_console_formatter() << errorTextPadded;
    pParentPanel_->JsEngineFail( errorTextPadded );
}

JsContainer::JsStatus JsContainer::GetStatus() const
{
    return jsStatus_;
}

bool JsContainer::ExecuteScript( const pfc::string8_fast& scriptCode )
{
    assert( pJsCtx_ );
    assert( jsGlobal_.initialized() );
    assert( JsStatus::Working == jsStatus_ );

    isParsingScript_ = true;

    JsScope autoScope( pJsCtx_, jsGlobal_ );

    JS::CompileOptions opts( pJsCtx_ );
    opts.setUTF8( true );
    opts.setFileAndLine( "<main>", 1 );

    JS::RootedValue dummyRval( pJsCtx_ );
    bool bRet = JS::Evaluate( pJsCtx_, opts, scriptCode.c_str(), scriptCode.length(), &dummyRval );

    isParsingScript_ = false;
    return bRet;
}

void JsContainer::RunJobs()
{    
    JsEngine::GetInstance().MaybeRunJobs();
}

void JsContainer::InvokeOnDragAction( const pfc::string8_fast& functionName, const POINTL& pt, uint32_t keyState, panel::DropActionParams& actionParams )
{
    if ( !IsReadyForCallback() )
    {
        return;
    }

    auto selfSaver = shared_from_this();
    JsScope autoScope( pJsCtx_, jsGlobal_ );

    if ( !CreateDropActionIfNeeded() )
    { // reports
        return;
    }

    pNativeDropAction_->GetDropActionParams() = actionParams;

    auto retVal = InvokeJsCallback( functionName,
                                    static_cast<JS::HandleObject>( jsDropAction_ ),
                                    static_cast<int32_t>( pt.x ),
                                    static_cast<int32_t>( pt.y ),
                                    static_cast<uint32_t>( keyState ) );
    if ( retVal )
    {
        actionParams = pNativeDropAction_->GetDropActionParams();
    }
}

void JsContainer::InvokeOnNotify( WPARAM wp, LPARAM lp )
{
    if ( !IsReadyForCallback() )
    {
        return;
    }

    auto selfSaver = shared_from_this();
    JsScope autoScope( pJsCtx_, jsGlobal_ );

    // Bind object to current compartment
    JS::RootedValue jsValue( pJsCtx_, *reinterpret_cast<JS::HandleValue*>( lp ) );
    if ( !JS_WrapValue( pJsCtx_, &jsValue ) )
    { // reports
        return;
    }

    autoScope.DisableReport(); ///< InvokeJsCallback has it's own AutoReportException
    (void)InvokeJsCallback( "on_notify_data",
                            *reinterpret_cast<std::wstring*>( wp ),
                            static_cast<JS::HandleValue>( jsValue ) );
    if ( jsValue.isObject() )
    {
        // TODO: test this! it was nuked only on success before
        // Remove binding
        js::NukeCrossCompartmentWrapper( pJsCtx_, &jsValue.toObject() );
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
    {// InvokeJsCallback invokes Fail() on error, which resets pNativeGraphics_
        pNativeGraphics_->SetGraphicsObject( nullptr );
    }
}

void JsContainer::InvokeJsAsyncTask( JsAsyncTask& jsTask )
{
    if ( !IsReadyForCallback() )
    {
        return;
    }

    auto selfSaver = shared_from_this();
    JsScope autoScope( pJsCtx_, jsGlobal_ );

    (void)jsTask.InvokeJs();
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

bool JsContainer::IsReadyForCallback() const
{
    return ( JsStatus::Working == jsStatus_ ) && !isParsingScript_;
}

} // namespace mozjs
