#include <stdafx.h>
#include "js_engine.h"

#include <js_engine/js_container.h>
#include <js_utils/js_error_helper.h>

#include <js_panel_window.h>

// TODO: remove js_panel_window, may be replace with HWND


namespace mozjs
{

JsEngine::JsEngine()
    : pJsCtx_( nullptr )
{
    isInitialized_ = false;
}

JsEngine::~JsEngine()
{
    // Attempt to cleanup, may result in crashes though, 
    // since mozjs.dll might be unloaded at this time    
    Finalize();
}

JsEngine& JsEngine::GetInstance()
{
    static JsEngine jsEnv;
    return jsEnv;
}

JSContext * JsEngine::GetJsContext() const
{
    assert( pJsCtx_ );
    return pJsCtx_;
}

bool JsEngine::RegisterPanel( js_panel_window& panel, JsContainer& jsContainer )
{
    if ( !registeredPanels_.size() )
    {
        if ( !Initialize() )
        {
            return false;
        }
    }

    if ( !jsContainer.Prepare( pJsCtx_, panel ) )
    {
        return false;
    }

    registeredPanels_.insert_or_assign( panel.GetHWND(), jsContainer);
    return true;
}

void JsEngine::UnregisterPanel( js_panel_window& parentPanel )
{
    auto elem = registeredPanels_.find( parentPanel.GetHWND() );
    if ( elem != registeredPanels_.end() )
    {
        elem->second.get().Finalize();
        registeredPanels_.erase( elem );
    }
    
    if ( !registeredPanels_.size() )
    {
        Finalize();
    }
}

bool JsEngine::Initialize()
{
    if ( isInitialized_ )
    {
        return true;
    }

    // TODO: Fine tune heap settings 
    JSContext* pJsCtx = JS_NewContext( 1024L * 1024 * 1024 );
    if (!pJsCtx)
    {
        return false;
    }

    std::unique_ptr<JSContext, void( *)(JSContext *)> autoJsCtx(
        pJsCtx,
        []( JSContext* pCtx )
    {
        JS_DestroyContext( pCtx );
    }
    );

    // TODO: JS::SetWarningReporter( pJsCtx_ )

    if (!JS::InitSelfHostedCode( pJsCtx ))
    {
        return false;
    }

#ifdef DEBUG
    JS_SetGCZeal( pJsCtx, 2, 20 );
#endif

    pJsCtx_ = autoJsCtx.release();
    isInitialized_ = true;
    return true;
}

void JsEngine::Finalize()
{
    if (pJsCtx_)
    {
        for each ( auto elem in registeredPanels_ )
        {
            elem.second.get().Finalize();
        }

        JS_DestroyContext( pJsCtx_ );
        pJsCtx_ = nullptr;
    }

    isInitialized_ = false;
}

}
