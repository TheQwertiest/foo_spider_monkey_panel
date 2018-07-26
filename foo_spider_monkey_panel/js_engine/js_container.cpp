#include <stdafx.h>
#include "js_container.h"

#include <js_engine/js_compartment_inner.h>
#include <js_objects/global_object.h>
#include <js_objects/gdi_graphics.h>
#include <js_objects/drop_source_action.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/scope_helper.h>

#include <js_panel_window.h>
#include <host_timer_dispatcher.h>


namespace mozjs
{

JsContainer::JsContainer()    
{
}

JsContainer::~JsContainer()
{
    Finalize();
    pJsCtx_ = nullptr;
}

bool JsContainer::Prepare( JSContext *cx, js_panel_window& parentPanel )
{
    assert( cx );

    pJsCtx_ = cx;
    pParentPanel_ = &parentPanel;
    jsStatus_ = JsStatus::Prepared;

    return Initialize();
}

bool JsContainer::Initialize()
{
    assert( JsStatus::NotPrepared != jsStatus_ );
    assert( pJsCtx_ );
    assert( pParentPanel_ );

    if ( JsStatus::Ready == jsStatus_ )
    {
        return true;
    }

    if ( jsGlobal_.initialized() || jsGraphics_.initialized() )
    {
        jsGraphics_.reset();
        jsGlobal_.reset();
    }

    JSAutoRequest ar( pJsCtx_ );

    jsGlobal_.init( pJsCtx_, JsGlobalObject::CreateNative( pJsCtx_, *this, *pParentPanel_ ) );
    if ( !jsGlobal_ )
    {
        return false;
    }

    JSAutoCompartment ac( pJsCtx_, jsGlobal_ );

    jsGraphics_.init( pJsCtx_, JsGdiGraphics::CreateJs( pJsCtx_ ) );
    if ( !jsGraphics_ )
    {
        jsGlobal_.reset();
        return false;
    }

    pNativeGlobal_ = static_cast<JsGlobalObject*>( JS_GetPrivate( jsGlobal_ ) );
    assert( pNativeGlobal_ );
    pNativeGraphics_ = static_cast<JsGdiGraphics*>(JS_GetPrivate( jsGraphics_ ));
    assert( pNativeGraphics_ );

    jsStatus_ = JsStatus::Ready;
    return true;
}

void JsContainer::Finalize()
{
    if ( JsStatus::NotPrepared == jsStatus_ )
    {
        return;
    }

    if ( JsStatus::Failed != jsStatus_ )
    {// Don't suppress error: it should be cleared only on initialization
        jsStatus_ = JsStatus::Prepared;
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

        JsGlobalObject::CleanupBeforeDestruction( pJsCtx_, jsGlobal_ );

        auto pJsCompartment = static_cast<JsCompartmentInner*>( JS_GetCompartmentPrivate( js::GetContextCompartment( pJsCtx_ ) ) );
        assert( pJsCompartment );

        pJsCompartment->MarkForDeletion();
    }

    jsGlobal_.reset();
}

void JsContainer::Fail()
{
    Finalize();
    jsStatus_ = JsStatus::Failed;
}

JsContainer::JsStatus JsContainer::GetStatus() const
{
    return jsStatus_;
}

bool JsContainer::ExecuteScript( const pfc::string8_fast&  scriptCode )
{
    assert( pJsCtx_ );
    assert( jsGlobal_.initialized() );
    assert( JsStatus::Ready == jsStatus_ );

    scope::JsScope autoScope( pJsCtx_, jsGlobal_ );

    JS::CompileOptions opts( pJsCtx_ );
    opts.setFileAndLine( "<main>", 1 );

    JS::RootedValue dummyRval( pJsCtx_ );    
    return JS::Evaluate( pJsCtx_, opts, scriptCode.c_str(), scriptCode.length(), &dummyRval );
}

void JsContainer::InvokeOnNotify( const std::wstring& name, const std::wstring& data )
{   
    if ( JsStatus::Ready != jsStatus_ )
    {
        return;
    }

    scope::JsScope autoScope( pJsCtx_, jsGlobal_ );

    JS::RootedValue jsStringVal( pJsCtx_ );
    if ( !convert::to_js::ToValue( pJsCtx_, data, &jsStringVal ) )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: on_notify_data received unparsable data" );
        return;
    }
    JS::RootedString jsString( pJsCtx_, jsStringVal.toString() );

    JS::RootedValue jsVal( pJsCtx_ );
    if ( !JS_ParseJSON( pJsCtx_, jsString, &jsVal ) )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: on_notify_data received unparsable data" );
        return;
    }

    autoScope.DisableReport(); ///< InvokeJsCallback has it's own AutoReportException
    InvokeJsCallback( "on_notify_data",
                      static_cast<std::wstring>(name),
                      static_cast<JS::HandleValue>(jsVal) );
}

void JsContainer::InvokeOnPaint( Gdiplus::Graphics& gr )
{
    if ( JsStatus::Ready != jsStatus_ )
    {
        return;
    }

    pNativeGraphics_->SetGraphicsObject( &gr );

    if ( !InvokeJsCallback( "on_paint",
                           static_cast<JS::HandleObject>(jsGraphics_) ) )
    {// Will clear pNativeGraphics_ on error through Fail
        return;
    }
    pNativeGraphics_->SetGraphicsObject( nullptr );
}

void JsContainer::InvokeOnDragAction( const pfc::string8_fast& functionName, const POINTL& pt, uint32_t keyState, DropActionParams& actionParams )
{
    if ( JsStatus::Ready != jsStatus_ )
    {
        return;
    }

    scope::JsScope autoScope( pJsCtx_, jsGlobal_ );

    if ( !CreateDropActionIfNeeded() )
    {// reports
        return;
    }

    pNativeDropAction_->GetDropActionParams() = actionParams;

    auto retVal = InvokeJsCallback( functionName,
                                    static_cast<JS::HandleObject>(jsDropAction_),
                                    static_cast<int32_t>(pt.x),
                                    static_cast<int32_t>(pt.y),
                                    static_cast<uint32_t>(keyState) );
    if ( retVal )
    {
        actionParams = pNativeDropAction_->GetDropActionParams();
    }
}

uint32_t JsContainer::SetInterval( HWND hWnd, uint32_t delay, JS::HandleFunction jsFunction )
{
    return HostTimerDispatcher::Get().setInterval( hWnd, delay, pJsCtx_, jsFunction );
}

uint32_t JsContainer::SetTimeout( HWND hWnd, uint32_t delay, JS::HandleFunction jsFunction )
{
    return HostTimerDispatcher::Get().setTimeout( hWnd, delay, pJsCtx_, jsFunction );
}

void JsContainer::KillTimer( uint32_t timerId )
{
    HostTimerDispatcher::Get().killTimer( timerId );
}

void JsContainer::InvokeTimerFunction( uint32_t timerId )
{
    HostTimerDispatcher::Get().onInvokeMessage( timerId );
}

bool JsContainer::CreateDropActionIfNeeded()
{
    if ( jsDropAction_.initialized() )
    {
        return true;
    }

    jsDropAction_.init( pJsCtx_, JsDropSourceAction::CreateJs( pJsCtx_ ) );
    if ( !jsDropAction_ )
    {// reports
        return false;
    }

    pNativeDropAction_ = static_cast<JsDropSourceAction*>(JS_GetPrivate( jsDropAction_ ));
    assert( pNativeDropAction_ );
    
    return true;
}

}
