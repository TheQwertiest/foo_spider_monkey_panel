#include <stdafx.h>
#include "js_engine.h"

#include <js_utils/js_error_helper.h>


namespace mozjs
{

JsEngine::JsEngine()
    : pJsCtx_( nullptr )
{
}

JsEngine::~JsEngine()
{
    Finalize();
}

JsEngine& JsEngine::GetInstance()
{
    static JsEngine jsEnv;
    return jsEnv;
}

bool JsEngine::RegisterPanel(HWND hPanel)
{
    if ( !registeredPanels_.size() )
    {
        if ( !Initialize() )
        {
            return false;
        }
    }

    registeredPanels_.insert( hPanel );
    return true;
}

void JsEngine::UnregisterPanel( HWND hPanel )
{
    registeredPanels_.erase( hPanel );
    if ( !registeredPanels_.size() )
    {
        Finalize();
    }
}

bool JsEngine::ExecuteScript( JS::HandleObject globalObject, std::string_view scriptCode )
{
    assert( pJsCtx_ );
    assert( !!globalObject );

    JSAutoRequest ar( pJsCtx_ );
    JSAutoCompartment ac( pJsCtx_, globalObject );
    
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

bool JsEngine::Initialize()
{
    Finalize();

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

    pJsCtx_ = autoJsCtx.release();

    return true;
}

void JsEngine::Finalize()
{
    if (pJsCtx_)
    {
        JS_DestroyContext( pJsCtx_ );
        pJsCtx_ = nullptr;
    }
}

JSContext * JsEngine::GetJsContext() const
{
    assert( pJsCtx_ );
    return pJsCtx_;
}

}
