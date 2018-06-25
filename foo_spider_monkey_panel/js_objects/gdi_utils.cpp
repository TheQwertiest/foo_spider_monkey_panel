#include <stdafx.h>

#include "gdi_utils.h"

#include <js_engine/js_value_converter.h>
#include <js_engine/js_native_invoker.h>
#include <js_engine/js_error_reporter.h>
#include <js_objects/js_object_wrapper.h>
#include <js_objects/gdi_font.h>
#include <js_objects/gdi_bitmap.h>
#include <js_utils/js_utils.h>

#include <helpers.h>


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

JSClass jsClass = {
    "GdiUtils",
    JSCLASS_HAS_PRIVATE | JSCLASS_FOREGROUND_FINALIZE,
    &jsOps
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiUtils, CreateImage )
MJS_DEFINE_JS_TO_NATIVE_FN_WITH_OPT( JsGdiUtils, Font, FontWithOpt, 1 )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "CreateImage", CreateImage, 2, 0 ),
    JS_FN( "Font", Font, 3, 0 ),
    JS_FS_END
};

}

namespace mozjs
{


JsGdiUtils::JsGdiUtils( JSContext* cx )
    : pJsCtx_( cx )
{
}


JsGdiUtils::~JsGdiUtils()
{
}

JSObject* JsGdiUtils::Create( JSContext* cx )
{
    JS::RootedObject jsObj( cx,
                            JS_NewObject( cx, &jsClass ) );
    if ( !jsObj )
    {
        return nullptr;
    }

    if ( !JS_DefineFunctions( cx, jsObj, jsFunctions ) )
    {
        return nullptr;
    }

    JS_SetPrivate( jsObj, new JsGdiUtils( cx ) );

    return jsObj;
}

const JSClass& JsGdiUtils::GetClass()
{
    return jsClass;
}

std::optional<JSObject*>
JsGdiUtils::CreateImage( uint32_t w, uint32_t h )
{
    std::unique_ptr<Gdiplus::Bitmap> img( new Gdiplus::Bitmap( w, h, PixelFormat32bppPARGB ) );
    if ( !helpers::ensure_gdiplus_object( img.get() ) )
    {
        JS_ReportErrorASCII( pJsCtx_, "Bitmap creation failed" );
        return std::nullopt;
    }

    JS::RootedObject jsObject( pJsCtx_, JsGdiBitmap::Create( pJsCtx_, img.get() ) );
    if ( !jsObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    img.release();
    return jsObject;
}

std::optional<JSObject*>
JsGdiUtils::Font( std::wstring fontName, float pxSize, uint32_t style )
{
    std::unique_ptr<Gdiplus::Font> pGdiFont( new Gdiplus::Font( fontName.c_str(), pxSize, style, Gdiplus::UnitPixel ) );
    if ( !helpers::ensure_gdiplus_object( pGdiFont.get() ) )
    {// Not an error: font not found
        return nullptr;
    }

    // Generate HFONT
    // The benefit of replacing Gdiplus::Font::GetLogFontW is that you can get it work with CCF/OpenType fonts.
    HFONT hFont = CreateFont(
        -(int)pxSize,
        0,
        0,
        0,
        ( style & Gdiplus::FontStyleBold ) ? FW_BOLD : FW_NORMAL,
        ( style & Gdiplus::FontStyleItalic ) ? TRUE : FALSE,
        ( style & Gdiplus::FontStyleUnderline ) ? TRUE : FALSE,
        ( style & Gdiplus::FontStyleStrikeout ) ? TRUE : FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        fontName.c_str() );

    JS::RootedObject jsRetObject( pJsCtx_, JsGdiFont::Create( pJsCtx_, pGdiFont.get(), hFont, true ) );
    if ( !jsRetObject )
    {
        DeleteObject( hFont );

        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    pGdiFont.release();
    return jsRetObject;
}

std::optional<JSObject*>
JsGdiUtils::FontWithOpt( size_t optArgCount, std::wstring fontName, float pxSize, uint32_t style )
{
    if ( optArgCount > 1 )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: invalid number of optional arguments specified: %d", optArgCount );
        return std::nullopt;
    }

    if ( optArgCount == 1 )
    {
        return Font( fontName, pxSize, 0 );
    }

    return Font( fontName, pxSize, style );
}

}
