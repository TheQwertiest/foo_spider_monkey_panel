#include <stdafx.h>

#include "js_engine.h"

// TODO: add error checking

namespace
{

static JSClassOps globalOps = {
     nullptr,
     nullptr,
     nullptr,
     nullptr,
     nullptr,
     nullptr,
     nullptr,
     nullptr,
     nullptr,
     nullptr,
     nullptr
};

static JSClass globalClass = {
     "global",
     JSCLASS_GLOBAL_FLAGS,
     &globalOps
};

}

namespace mozjs
{

JsEngine::JsEngine()
     : pJsCtx_( nullptr )   
     , globalObjectCount_(0)
{
     globalOps.trace = JS_GlobalObjectTraceHook;
}

JsEngine::~JsEngine()
{
     Finalize();
}

void JsEngine::Initialize()
{
     Finalize();

     // TODO: Fine tune heap settings 
     JSContext* pJsCtx = JS_NewContext( 1024L * 1024 * 1024 );
     if ( !pJsCtx )
     {
          return;
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
          return;
     }

     pJsCtx_ = autoJsCtx.release();
}

void JsEngine::Finalize()
{
     if ( pJsCtx_ )
     {
          JS_DestroyContext( pJsCtx_ );
          pJsCtx_ = NULL;
     }
}

void JsEngine::ExecuteScript( JS::HandleObject globalObject )
{
     JSAutoRequest ar( pJsCtx_ );
     JS::RootedValue rval( pJsCtx_ );
     {
          JSAutoCompartment ac( pJsCtx_, globalObject );

          const char *script = "'hello'+'world, it is '+new Date()";
          const char *filename = "noname";
          int lineno = 1;
          JS::CompileOptions opts( pJsCtx_ );
          opts.setFileAndLine( filename, lineno );
          bool ok = JS::Evaluate( pJsCtx_, opts, script, strlen( script ), &rval );
          if ( !ok )
          {
               console::printf( JSP_NAME " Evaluate faul =(\n" );
               return;
          }
     }

     JSString *str = rval.toString();
     console::printf( JSP_NAME " (%s)\n", JS_EncodeString( pJsCtx_, str ) );
}

JsEngine& JsEngine::GetInstance()
{
     static JsEngine jsEnv;
     return jsEnv;
}

void JsEngine::CreateGlobalObject( JS::PersistentRootedObject& globalObject )
{
     if ( !globalObjectCount_ )
     {
          Initialize();
     }

     JSAutoRequest ar( pJsCtx_ );
     JS::CompartmentOptions options;  
     JS::RootedObject glob( pJsCtx_, 
                            JS_NewGlobalObject( pJsCtx_, &globalClass, nullptr, JS::DontFireOnNewGlobalHook, options ) );
     if ( !glob )
     {
          return;
     }

     {
          JSAutoCompartment ac( pJsCtx_, glob );

          if ( !JS_InitStandardClasses( pJsCtx_, glob ) )
          {
               return;
          }

          JS_FireOnNewGlobalObject( pJsCtx_, glob );
     }

     globalObject.init( pJsCtx_, glob );
     ++globalObjectCount_;
}

void JsEngine::DestroyGlobalObject( JS::PersistentRootedObject& globalObject )
{
     if ( !globalObject.initialized() )
     {
          return;
     }

     globalObject.reset();
     --globalObjectCount_;

     if ( !globalObjectCount_ )
     {
          Finalize();
     }
}

}