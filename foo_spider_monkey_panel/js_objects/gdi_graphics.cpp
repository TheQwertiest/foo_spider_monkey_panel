#include <stdafx.h>

#include "gdi_graphics.h"

#include <js_engine/js_value_converter.h>

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
    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );

    if ( args.length() != 6 )
    {
        //JS_ReportErrorNumberASCII( cx, my_GetErrorMessage, nullptr,
                                   //args.length() < 1 ? JSSMSG_NOT_ENOUGH_ARGS : JSSMSG_TOO_MANY_ARGS,
                                   //"evaluate" );
        return false;
    }

    float x, y, w, h, line_width;
    uint32_t colour;

    if ( !UnwrapValue( args[0], x ) 
         || !UnwrapValue( args[1], y ) 
         || !UnwrapValue( args[2], w )
         || !UnwrapValue( args[3], h ) 
         || !UnwrapValue( args[4], line_width ) 
         || !UnwrapValue( args[5], colour ) )
    {
        return false;
    }

    
    auto pGdiGraphics = static_cast<JsGdiGraphics*>( JS_GetPrivate( args.thisv().toObjectOrNull() ) );
    if ( !pGdiGraphics )
    {
        return false;
    }

    return pGdiGraphics->DrawRect(x, y, w, h, line_width, colour);
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
