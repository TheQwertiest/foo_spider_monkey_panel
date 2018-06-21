#include <stdafx.h>

#include "js_global_object.h"
#include "js_console.h"

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

JSObject* CreateGlobalObject( JSContext* cx )
{
     if ( !globalOps.trace )
     {// JS_GlobalObjectTraceHook address is only accessible after mozjs is loaded.      
          globalOps.trace = JS_GlobalObjectTraceHook;
     }

     JSAutoRequest ar( cx );
     JS::CompartmentOptions options;
     JS::RootedObject glob( cx,
                            JS_NewGlobalObject( cx, &globalClass, nullptr, JS::DontFireOnNewGlobalHook, options ) );
     if ( !glob )
     {
          return nullptr;
     }

     {
          JSAutoCompartment ac( cx, glob );

          if ( !JS_InitStandardClasses( cx, glob ) )
          {
               return nullptr;
          }

          if ( !DefineConsole( cx, glob ) )
          {
               return nullptr;
          }

          JS_FireOnNewGlobalObject( cx, glob );
     }

     return glob;
}

}
