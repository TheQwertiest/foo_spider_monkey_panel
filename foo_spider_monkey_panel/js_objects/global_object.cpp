#include <stdafx.h>

#include "global_object.h"
#include "console.h"

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

// TODO: remove HWND and HAS_PRIVATE after creating Window class

static JSClass globalClass = {
     "global",
     JSCLASS_GLOBAL_FLAGS | JSCLASS_HAS_PRIVATE,
     &globalOps
};

}

namespace mozjs
{


JsGlobalObject::JsGlobalObject( JSContext* cx, HWND hParentPanel )
    : pJsCtx_( cx )
    , hParentPanel_( hParentPanel )
    , hasFailed_( false)
{
}


JsGlobalObject::~JsGlobalObject()
{
}

JSObject* JsGlobalObject::Create( JSContext* cx, HWND hParentPanel )
{
    if ( !globalOps.trace )
    {// JS_GlobalObjectTraceHook address is only accessible after mozjs is loaded.      
        globalOps.trace = JS_GlobalObjectTraceHook;
    }

    JS::CompartmentOptions options;
    JS::RootedObject jsObj( cx,
                            JS_NewGlobalObject( cx, &globalClass, nullptr, JS::DontFireOnNewGlobalHook, options ) );
    if ( !jsObj )
    {
        return nullptr;
    }

    {
        JSAutoCompartment ac( cx, jsObj );

        if ( !JS_InitStandardClasses( cx, jsObj ) )
        {
            return nullptr;
        }

        if ( !DefineConsole( cx, jsObj ) )
        {
            return nullptr;
        }

        JS_SetPrivate( jsObj, new JsGlobalObject( cx, hParentPanel ) );

        JS_FireOnNewGlobalObject( cx, jsObj );
    }

    return jsObj;
}

HWND JsGlobalObject::GetHWND() const
{
    return hParentPanel_;
}

bool JsGlobalObject::HasFailed() const
{
    return hasFailed_;
}

void JsGlobalObject::Fail()
{
    hasFailed_ = true;
}

}
