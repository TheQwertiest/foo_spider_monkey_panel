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

bool JsEngine::ExecuteScript( JS::HandleObject globalObject, std::string_view scriptCode )
{
     JSAutoRequest ar( pJsCtx_ );
     JS::RootedValue rval( pJsCtx_ );
     {
          JSAutoCompartment ac( pJsCtx_, globalObject );
          
          const char *filename = "noname";
          int lineno = 1;
          JS::CompileOptions opts( pJsCtx_ );
          opts.setFileAndLine( filename, lineno );
          bool ok = JS::Evaluate( pJsCtx_, opts, scriptCode.data(), scriptCode.length(), &rval );
          if ( !ok )
          {
               JS_ClearPendingException( pJsCtx_ );
               console::printf( JSP_NAME " Evaluate faul =(\n" );
               return false;;
          }
     }

     return true;
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

}
