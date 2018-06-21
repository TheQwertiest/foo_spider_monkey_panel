#include <stdafx.h>

#include "js_engine.h"
#include "js_global_object.h"

#include <js/Conversions.h>

// TODO: think about moving context creation somewhere above 
// to avoid it's recreation when working with single panel

namespace mozjs
{

JsEngine::JsEngine()
     : pJsCtx_( nullptr )   
     , globalObjectCount_(0)
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

bool JsEngine::CreateGlobalObject( JS::PersistentRootedObject& globalObject )
{
     if ( !globalObjectCount_ )
     {
          if ( !Initialize() )
          {
               return false;
          }
     }

     JS::RootedObject newGlobal( pJsCtx_, mozjs::CreateGlobalObject( pJsCtx_ ) );
     if ( !newGlobal )
     {
          return false;
     }

     globalObject.init( pJsCtx_, newGlobal );
     ++globalObjectCount_;

     return true;
}

void JsEngine::DestroyGlobalObject( JS::PersistentRootedObject& globalObject )
{
     if ( !globalObject.initialized() )
     {
          return;
     }

     assert( globalObjectCount_ > 0 );

     globalObject.reset();
     --globalObjectCount_;

     if ( !globalObjectCount_ )
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

     const char *filename = "noname";
     int lineno = 1;
     JS::CompileOptions opts( pJsCtx_ );
     opts.setFileAndLine( filename, lineno );

     JS::RootedValue rval( pJsCtx_ );
     bool bRet = JS::Evaluate( pJsCtx_, opts, scriptCode.data(), scriptCode.length(), &rval );
     if ( !bRet )
     {
          JS_ClearPendingException( pJsCtx_ );
          console::printf( JSP_NAME "JS::Evaluate failed\n" );
          return false;
     }

     JS::AutoReportException

     return true;
}

bool JsEngine::InbokeCallback( std::string_view functionName, JS::HandleObject globalObject,
                               const JS::HandleValueArray& args, JS::MutableHandleValue rval )
{
     assert( pJsCtx_ );
     assert( !!globalObject );
     assert( functionName.length() );

     JSAutoRequest ar( pJsCtx_ );
     JSAutoCompartment ac( pJsCtx_, globalObject );

     JS::RootedValue funcValue( pJsCtx_ );
     if ( !JS_GetProperty( pJsCtx_, globalObject, functionName.data(), &funcValue ) )
     {
          return false;
     }

     if ( funcValue.isUndefined() )
     {// not an error
          return true;
     }

     JS::RootedFunction func( pJsCtx_, JS_ValueToFunction(pJsCtx_, funcValue ) );
     if ( !func )
     {
          return false;
     }

     if ( !JS::Call( pJsCtx_, globalObject, func, args, rval ) )
     {
          JS_ClearPendingException( pJsCtx_ );
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
     if ( !pJsCtx )
     {
          return false;
     }

     std::unique_ptr<JSContext, void( *)( JSContext * )> autoJsCtx(
          pJsCtx,
          []( JSContext* pCtx )
          {
               JS_DestroyContext( pCtx );
          }
     );

     // TODO: JS::SetWarningReporter( pJsCtx_ )

     if ( !JS::InitSelfHostedCode( pJsCtx ) )
     {
          return false;
     }

     pJsCtx_ = autoJsCtx.release();

     return true;
}

void JsEngine::Finalize()
{
     if ( pJsCtx_ )
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
