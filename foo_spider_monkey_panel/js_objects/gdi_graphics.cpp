#include <stdafx.h>

#include "gdi_graphics.h"

#include <js_engine/js_value_converter.h>
#include <js_engine/js_native_invoker.h>
#include <js_engine/js_error_reporter.h>

#define MOZJS_DEFINE_JS_TO_NATIVE_CALLBACK(baseClass, functionName) \
    bool functionName( JSContext* cx, unsigned argc, JS::Value* vp )\
    {\
        Mjs_Status mjsRet = InvokeNativeCallback( cx, &baseClass::functionName, argc, vp );\
        if (Mjs_Ok != mjsRet)\
        {\
            JS_ReportErrorASCII( cx, ErrorCodeToString( mjsRet ) );\
        }\
        return Mjs_Ok == mjsRet;\
    }

#define IF_GDI_FAILED_RETURN(x,y) \
    do \
    {\
        if ( x > 0 )\
        {\
            return y;\
        }\
    } while(false)

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

MOZJS_DEFINE_JS_TO_NATIVE_CALLBACK( JsGdiGraphics, DrawEllipse )
MOZJS_DEFINE_JS_TO_NATIVE_CALLBACK( JsGdiGraphics, DrawLine )
MOZJS_DEFINE_JS_TO_NATIVE_CALLBACK( JsGdiGraphics, DrawRect )
MOZJS_DEFINE_JS_TO_NATIVE_CALLBACK( JsGdiGraphics, DrawRoundRect )
MOZJS_DEFINE_JS_TO_NATIVE_CALLBACK( JsGdiGraphics, FillEllipse )
MOZJS_DEFINE_JS_TO_NATIVE_CALLBACK( JsGdiGraphics, FillGradRect )
MOZJS_DEFINE_JS_TO_NATIVE_CALLBACK( JsGdiGraphics, FillRoundRect )
MOZJS_DEFINE_JS_TO_NATIVE_CALLBACK( JsGdiGraphics, FillSolidRect )

static const JSFunctionSpec gdiGraphicsFunctions[] = {
    JS_FN( "DrawEllipse", DrawEllipse, 6, 0 ),
    JS_FN( "DrawLine", DrawLine, 6, 0 ),
    JS_FN( "DrawRect", DrawRect, 6, 0 ),
    JS_FN( "DrawRoundRect", DrawRoundRect, 8, 0 ),
    JS_FN( "FillEllipse", FillEllipse, 5, 0 ),
    JS_FN( "FillGradRect", FillGradRect, 8, 0 ),
    JS_FN( "FillRoundRect", FillRoundRect, 7, 0 ),
    JS_FN( "FillSolidRect", FillSolidRect, 5, 0 ),
    JS_FS_END
};

}

namespace mozjs
{


JsGdiGraphics::JsGdiGraphics( JSContext* cx )
    : pJsCtx_( cx )
    , graphics_( NULL )
{
}


JsGdiGraphics::~JsGdiGraphics()
{
}

JSObject* JsGdiGraphics::Create( JSContext* cx )
{
    JS::RootedObject jsObj( cx,
                            JS_NewObject( cx, &gdiGraphicsClass ) );
    if ( !jsObj )
    {
        return nullptr;
    }

    if ( !JS_DefineFunctions( cx, jsObj, gdiGraphicsFunctions ) )
    {
        return nullptr;
    }

    JS_SetPrivate( jsObj, new JsGdiGraphics( cx ) );

    return jsObj;
}

void JsGdiGraphics::SetGraphicsObject( Gdiplus::Graphics* graphics )
{
    graphics_ = graphics;
}

Mjs_Status JsGdiGraphics::DrawEllipse( float x, float y, float w, float h, float line_width, uint32_t colour )
{
    if ( !graphics_ )
    {
        return Mjs_EngineInternalError;
    }

    Gdiplus::Pen pen( colour, line_width );
    Gdiplus::Status gdiRet = graphics_->DrawEllipse( &pen, x, y, w, h );
    IF_GDI_FAILED_RETURN( gdiRet, Mjs_InternalError );

    return Mjs_Ok;
}

Mjs_Status JsGdiGraphics::DrawLine( float x1, float y1, float x2, float y2, float line_width, uint32_t colour )
{
    if ( !graphics_ )
    {
        return Mjs_EngineInternalError;
    }

    Gdiplus::Pen pen( colour, line_width );
    Gdiplus::Status gdiRet = graphics_->DrawLine( &pen, x1, y1, x2, y2 );
    IF_GDI_FAILED_RETURN( gdiRet, Mjs_InternalError );

    return Mjs_Ok;
}

Mjs_Status JsGdiGraphics::DrawRect( float x, float y, float w, float h, float line_width, uint32_t colour )
{
    if ( !graphics_ )
    {
        return Mjs_EngineInternalError;
    }

    Gdiplus::Pen pen( colour, line_width );
    Gdiplus::Status gdiRet = graphics_->DrawRectangle( &pen, x, y, w, h );
    IF_GDI_FAILED_RETURN( gdiRet, Mjs_InternalError );

    return Mjs_Ok;
}

Mjs_Status JsGdiGraphics::DrawRoundRect( float x, float y, float w, float h, float arc_width, float arc_height, float line_width, uint32_t colour )
{
    if ( !graphics_ )
    {
        return Mjs_EngineInternalError;
    }

    if ( 2 * arc_width > w || 2 * arc_height > h )
    {
        return Mjs_InvalidArgumentValue;
    }

    Gdiplus::Pen pen( colour, line_width );
    Gdiplus::GraphicsPath gp;
    Gdiplus::RectF rect( x, y, w, h );
    Gdiplus::Status gdiRet = (Gdiplus::Status)GetRoundRectPath( gp, rect, arc_width, arc_height );
    IF_GDI_FAILED_RETURN( gdiRet, Mjs_InternalError );

    gdiRet = pen.SetStartCap( Gdiplus::LineCapRound );
    IF_GDI_FAILED_RETURN( gdiRet, Mjs_InternalError );

    gdiRet = pen.SetEndCap( Gdiplus::LineCapRound );
    IF_GDI_FAILED_RETURN( gdiRet, Mjs_InternalError );

    gdiRet = graphics_->DrawPath( &pen, &gp );
    IF_GDI_FAILED_RETURN( gdiRet, Mjs_InternalError );

    return Mjs_Ok;
}

Mjs_Status JsGdiGraphics::FillEllipse( float x, float y, float w, float h, uint32_t colour )
{
    if ( !graphics_ )
    {
        return Mjs_EngineInternalError;
    }

    Gdiplus::SolidBrush br( colour );
    Gdiplus::Status gdiRet = graphics_->FillEllipse( &br, x, y, w, h );
    IF_GDI_FAILED_RETURN( gdiRet, Mjs_InternalError );

    return Mjs_Ok;
}

Mjs_Status JsGdiGraphics::FillGradRect( float x, float y, float w, float h, float angle, uint32_t colour1, uint32_t colour2, float focus )
{
    if ( !graphics_ )
    {
        return Mjs_EngineInternalError;
    }

    Gdiplus::RectF rect( x, y, w, h );
    Gdiplus::LinearGradientBrush brush( rect, colour1, colour2, angle, TRUE );
    Gdiplus::Status gdiRet = brush.SetBlendTriangularShape( focus );
    IF_GDI_FAILED_RETURN( gdiRet, Mjs_InternalError );

    gdiRet = graphics_->FillRectangle( &brush, rect );
    IF_GDI_FAILED_RETURN( gdiRet, Mjs_InternalError );

    return Mjs_Ok;
}

Mjs_Status JsGdiGraphics::FillRoundRect( float x, float y, float w, float h, float arc_width, float arc_height, uint32_t colour )
{
    if ( !graphics_ )
    {
        return Mjs_EngineInternalError;
    }

    if ( 2 * arc_width > w || 2 * arc_height > h )
    {
        return Mjs_InvalidArgumentValue;
    }

    Gdiplus::SolidBrush br( colour );
    Gdiplus::GraphicsPath gp;
    Gdiplus::RectF rect( x, y, w, h );
    Gdiplus::Status gdiRet = (Gdiplus::Status)GetRoundRectPath( gp, rect, arc_width, arc_height );
    IF_GDI_FAILED_RETURN( gdiRet, Mjs_InternalError );

    gdiRet = graphics_->FillPath( &br, &gp );
    IF_GDI_FAILED_RETURN( gdiRet, Mjs_InternalError );

    return Mjs_Ok;
}

Mjs_Status JsGdiGraphics::FillSolidRect( float x, float y, float w, float h, uint32_t colour )
{
    if ( !graphics_ )
    {
        return Mjs_EngineInternalError;
    }

    Gdiplus::SolidBrush brush( colour );
    Gdiplus::Status gdiRet = graphics_->FillRectangle( &brush, x, y, w, h );
    IF_GDI_FAILED_RETURN( gdiRet, Mjs_InternalError );

    return Mjs_Ok;
}

int JsGdiGraphics::GetRoundRectPath( Gdiplus::GraphicsPath& gp, Gdiplus::RectF& rect, float arc_width, float arc_height )
{
    float arc_dia_w = arc_width * 2;
    float arc_dia_h = arc_height * 2;
    Gdiplus::RectF corner( rect.X, rect.Y, arc_dia_w, arc_dia_h );

    Gdiplus::Status gdiRet = gp.Reset();
    IF_GDI_FAILED_RETURN( gdiRet, gdiRet );

    // top left
    gdiRet = gp.AddArc( corner, 180, 90 );
    IF_GDI_FAILED_RETURN( gdiRet, gdiRet );

    // top right
    corner.X += (rect.Width - arc_dia_w);
    gdiRet = gp.AddArc( corner, 270, 90 );
    IF_GDI_FAILED_RETURN( gdiRet, gdiRet );

    // bottom right
    corner.Y += (rect.Height - arc_dia_h);
    gdiRet = gp.AddArc( corner, 0, 90 );
    IF_GDI_FAILED_RETURN( gdiRet, gdiRet );

    // bottom left
    corner.X -= (rect.Width - arc_dia_w);
    gdiRet = gp.AddArc( corner, 90, 90 );
    IF_GDI_FAILED_RETURN( gdiRet, gdiRet );

    gdiRet = gp.CloseFigure();
    IF_GDI_FAILED_RETURN( gdiRet, gdiRet );

    return Gdiplus::Ok;
}

}
