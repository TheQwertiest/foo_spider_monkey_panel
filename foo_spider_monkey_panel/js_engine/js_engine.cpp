#include <stdafx.h>

#include "js_engine.h"

#include <js/Conversions.h>

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

static bool
Log( JSContext* cx, unsigned argc, JS::Value* vp )
{
     JS::CallArgs args = JS::CallArgsFromVp( argc, vp );

     std::string outputString;

     for ( unsigned i = 0; i < args.length(); i++ )
     {
          JS::RootedString str( cx, JS::ToString( cx, args[i] ) );
          if ( !str )
          {
               return false;
          }

          char* bytes = JS_EncodeStringToUTF8( cx, str );
          if ( !bytes )
          {
               return false;
          }

          outputString += bytes;

          JS_free( cx, bytes );
     }

     args.rval().setUndefined();

     console::printf( outputString.c_str() );
     return true;
}

static const JSFunctionSpec console_functions[] = {
     JS_FN( "log", Log, 0, 0 ),
     JS_FS_END
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

void JsEngine::ExecuteScript( JS::HandleObject globalObject, std::string_view scriptCode )
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
               return;
          }
     }
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

          JS::RootedObject consoleObj( pJsCtx_, JS_NewPlainObject( pJsCtx_ ) );
          if ( !consoleObj
               || !JS_DefineFunctions( pJsCtx_, consoleObj, console_functions )
               || !JS_DefineProperty( pJsCtx_, glob, "console", consoleObj, 0 ) )
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