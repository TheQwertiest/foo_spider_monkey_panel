#include <stdafx.h>
#include "global_object.h"

#include <js_engine/js_container.h>
#include <js_objects/console.h>
#include <js_objects/gdi_utils.h>
#include <js_objects/active_x.h>
#include <js_utils/js_object_helper.h>

#include <js_panel_window.h>

#include <js/TracingAPI.h>

namespace
{

using namespace mozjs;

JSClassOps jsOps = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    JsFinalizeOp<JsGdiUtils>,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

// TODO: remove HWND and HAS_PRIVATE after creating Window class

JSClass jsClass = {
     "Global",
     JSCLASS_GLOBAL_FLAGS | JSCLASS_HAS_PRIVATE | JSCLASS_FOREGROUND_FINALIZE,
     &jsOps
};

}

namespace mozjs
{


JsGlobalObject::JsGlobalObject( JSContext* cx, JsContainer &parentContainer, js_panel_window& parentPanel )
    : pJsCtx_( cx )
    , parentContainer_( parentContainer )
    , parentPanel_( parentPanel )
{
}


void JsGlobalObject::TraceHeapValue( JSTracer *trc, void *data )
{
    assert( data );
    auto globalObject = static_cast<JsGlobalObject*>( data );
    
    for each ( auto elem in globalObject->tracerMap_ )
    {
        JS::TraceEdge( trc, elem.first, "CustomHeap" );
    }
}

JsGlobalObject::~JsGlobalObject()
{
    JS_RemoveExtraGCRootsTracer( pJsCtx_, JsGlobalObject::TraceHeapValue, this );
}

JSObject* JsGlobalObject::Create( JSContext* cx, JsContainer &parentContainer, js_panel_window& parentPanel )
{
    if ( !jsOps.trace )
    {// JS_GlobalObjectTraceHook address is only accessible after mozjs is loaded.      
        jsOps.trace = JS_GlobalObjectTraceHook;
    }

    JS::CompartmentOptions options;
    JS::RootedObject jsObj( cx,
                            JS_NewGlobalObject( cx, &jsClass, nullptr, JS::DontFireOnNewGlobalHook, options ) );
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

        JS::RootedObject gdiObj( cx, JsGdiUtils::Create( cx ) );
        if ( !gdiObj )
        {
            return nullptr;
        }

        if ( !JS_DefineProperty( cx, jsObj, "gdi", gdiObj, 0 ) )
        {
            return nullptr;
        }

        JS::RootedObject activeXproto( cx, CreateActiveXProto( cx, jsObj ) );
        if ( !activeXproto )
        {
            return nullptr;
        }

        JsGlobalObject* pNative = new JsGlobalObject( cx, parentContainer, parentPanel );
        JS_SetPrivate( jsObj, pNative );

        if ( !JS_AddExtraGCRootsTracer( cx, JsGlobalObject::TraceHeapValue, pNative ) )
        {
            return nullptr;
        }

        JS_FireOnNewGlobalObject( cx, jsObj );
    }

    return jsObj;
}

const JSClass& JsGlobalObject::GetClass()
{
    return jsClass;
}

void JsGlobalObject::Fail( std::string_view errorText )
{
    parentContainer_.Fail();
    parentPanel_.JsEngineFail( errorText );
}

void JsGlobalObject::AddHeapToTrace( JS::Heap<JS::Value>* heapValue )
{
    assert( tracerMap_.end() == tracerMap_.find( heapValue ) );
    tracerMap_[heapValue] = heapValue;
}

void JsGlobalObject::RemoveHeapFromTrace( JS::Heap<JS::Value>* heapValue )
{
    assert( tracerMap_.end() != tracerMap_.find( heapValue ) );
    tracerMap_.erase( heapValue );
}

}
