#include <stdafx.h>
#include "js_container.h"

#include <js_objects/global_object.h>
#include <js_objects/gdi_graphics.h>
#include <js_objects/window.h>
#include <js_utils/js_error_helper.h>


// TODO: remove js_panel_window

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

    jsGlobal_.init( pJsCtx_, JsGlobalObject::Create( pJsCtx_, *this, *pParentPanel_ ) );
    if ( !jsGlobal_ )
    {
        return false;
    }

    JSAutoCompartment ac( pJsCtx_, jsGlobal_ );

    jsGraphics_.init( pJsCtx_, JsGdiGraphics::Create( pJsCtx_ ) );
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
    {// Don't supress error: it should be cleared only on initialization
        jsStatus_ = JsStatus::Prepared;
    }
    
    pNativeGraphics_ = nullptr;
    jsGraphics_.reset();
    if ( !jsGlobal_.initialized() )
    {
        return;
    }

    {
        JSAutoRequest ar( pJsCtx_ );
        JSAutoCompartment ac( pJsCtx_, jsGlobal_ );

        JS::RootedValue jsProperty( pJsCtx_ );
        if ( JS_GetProperty( pJsCtx_, jsGlobal_, "window", &jsProperty ) )
        {
            JS::RootedObject jsWindow( pJsCtx_, jsProperty.toObjectOrNull() );
            if ( jsWindow )
            {
                auto nativeWindow = static_cast<JsWindow*>( JS_GetInstancePrivate( pJsCtx_, jsWindow, &JsWindow::GetClass(), nullptr ) );
                if ( nativeWindow )
                {
                    nativeWindow->RemoveHeapTracer();
                }
            }
        }
    }

    auto nativeGlobal_ = static_cast<JsGlobalObject*>( JS_GetPrivate( jsGlobal_ ) );
    nativeGlobal_->RemoveHeapTracer();

    jsGlobal_.reset();
}

void JsContainer::Fail()
{
    Finalize();
    jsStatus_ = JsStatus::Failed;
}

bool JsContainer::ExecuteScript( std::string_view scriptCode )
{
    assert( pJsCtx_ );
    assert( jsGlobal_.initialized() );
    assert( JsStatus::Ready == jsStatus_ );

    JSAutoRequest ar( pJsCtx_ );
    JSAutoCompartment ac( pJsCtx_, jsGlobal_ );

    JS::CompileOptions opts( pJsCtx_ );
    opts.setFileAndLine( "<main>", 1 );

    JS::RootedValue rval( pJsCtx_ );

    AutoReportException are( pJsCtx_ );
    bool bRet = JS::Evaluate( pJsCtx_, opts, scriptCode.data(), scriptCode.length(), &rval );
    if ( !bRet )
    {
        console::printf( JSP_NAME "JS::Evaluate failed\n" );
        return false;
    }

    return true;
}

JsContainer::JsStatus JsContainer::GetStatus() const
{
    return jsStatus_;
}

JS::HandleObject JsContainer::GetGraphics() const
{
    return jsGraphics_;
}

JsContainer::GraphicsWrapper::GraphicsWrapper( JsContainer& parent, Gdiplus::Graphics& gr )
    :parent_( parent )
{// TODO: remove this awkward wrapper
    assert( JsStatus::Ready == parent_.jsStatus_ );
    assert( parent_.pNativeGraphics_ );

    parent_.pNativeGraphics_->SetGraphicsObject( &gr );
}

JsContainer::GraphicsWrapper::~GraphicsWrapper()
{
    if ( JsStatus::Ready == parent_.jsStatus_ )
    {
        parent_.pNativeGraphics_->SetGraphicsObject( nullptr );
    }
}

}
