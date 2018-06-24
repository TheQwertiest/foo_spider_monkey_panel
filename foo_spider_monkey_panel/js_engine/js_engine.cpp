#include <stdafx.h>

#include "js_engine.h"
#include "js_error_reporter.h"
#include <js_objects/global_object.h>

#include <js/Conversions.h>


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
        if ( Initialize() )
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

bool JsEngine::InvokeCallbackInternal( JS::HandleObject globalObject,
                                       std::string_view functionName,
                                       const JS::HandleValueArray& args,
                                       JS::MutableHandleValue rval )
{
    assert( pJsCtx_ );
    assert( !!globalObject );
    assert( functionName.length() );

    JSAutoRequest ar( pJsCtx_ );
    JSAutoCompartment ac( pJsCtx_, globalObject );

    JS::RootedValue funcValue( pJsCtx_ );
    if (!JS_GetProperty( pJsCtx_, globalObject, functionName.data(), &funcValue ))
    {
        return false;
    }

    if (funcValue.isUndefined())
    {// not an error
        return true;
    }

    JS::RootedFunction func( pJsCtx_, JS_ValueToFunction( pJsCtx_, funcValue ) );
    if (!func)
    {
        return false;
    }

    AutoReportException are( pJsCtx_ );
    if (!JS::Call( pJsCtx_, globalObject, func, args, rval ))
    {        
        console::printf( JSP_NAME "JS::JS_Call failed\n" );
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
        pJsCtx_ = NULL;
    }
}

JSContext * JsEngine::GetJsContext() const
{
    assert( pJsCtx_ );
    return pJsCtx_;
}

}
