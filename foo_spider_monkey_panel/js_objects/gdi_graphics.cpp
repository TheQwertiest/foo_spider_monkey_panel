#include <stdafx.h>

#include "gdi_graphics.h"

#include <js_engine/js_value_wrapper.h>
#include <js_engine/js_native_invoker.h>

namespace
{

using namespace mozjs;

static JSClassOps gdiGraphicsOps = {
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

static JSClass gdiGraphicsClass = {
    "GdiGraphics",
    JSCLASS_HAS_PRIVATE,
    &gdiGraphicsOps
};

bool DrawRect( JSContext* cx, unsigned argc, JS::Value* vp )
{
    return InvokeNativeCallback<
        JsGdiGraphics
    >( cx, &JsGdiGraphics::DrawRect, argc, vp );
}

static const JSFunctionSpec gdiGraphicsFunctions[] = {
    JS_FN( "DrawRect", DrawRect, 6, 0 ),
    JS_FS_END
};

}

namespace mozjs
{


JsGdiGraphics::JsGdiGraphics( JSContext* cx )
    : pJsCtx_(cx)    
{
}


JsGdiGraphics::~JsGdiGraphics()
{
}

JSObject* JsGdiGraphics::Create( JSContext* cx, JS::HandleObject global )
{
    JSAutoRequest ar( cx );
    JS::CompartmentOptions options;
    JS::RootedObject jsObj( cx,
                           JS_NewObject( cx, &gdiGraphicsClass ) );
    if ( !jsObj )
    {
        return nullptr;
    }

    {
        JSAutoCompartment ac( cx, global );

        if ( !JS_DefineFunctions( cx, jsObj, gdiGraphicsFunctions ) )
        {
            return nullptr;
        }

        JS_SetPrivate( jsObj, new JsGdiGraphics( cx ) );
    }    

    return jsObj;
}

void JsGdiGraphics::SetGraphicsObject( Gdiplus::Graphics* graphics )
{
    graphics_.reset( graphics );
}

bool JsGdiGraphics::DrawRect( float x, float y, float w, float h, float line_width, uint32_t colour )
{
    if ( !graphics_ )
    {
        return false;
    }

    Gdiplus::Pen pen( colour, line_width );
    graphics_->DrawRectangle( &pen, x, y, w, h );
    
    return true;
}

}
