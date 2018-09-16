#include <stdafx.h>

#include "gdi_utils.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/gdi_font.h>
#include <js_objects/gdi_bitmap.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/gdi_helpers.h>
#include <js_utils/scope_helper.h>
#include <js_utils/image_helper.h>
#include <js_utils/winapi_error_helper.h>

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
    JsGdiUtils::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "GdiUtils",
    DefaultClassFlags(),
    &jsOps
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiUtils, CreateImage )
MJS_DEFINE_JS_TO_NATIVE_FN_WITH_OPT( JsGdiUtils, Font, FontWithOpt, 1 )
MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiUtils, Image )
MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiUtils, LoadImageAsync )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "CreateImage", CreateImage, 2, DefaultPropsFlags() ),
    JS_FN( "Font", Font, 2, DefaultPropsFlags() ),
    JS_FN( "Image", Image, 1, DefaultPropsFlags() ),
    JS_FN( "LoadImageAsync", LoadImageAsync, 2, DefaultPropsFlags() ),
    JS_FS_END
};

const JSPropertySpec jsProperties[] = {
    JS_PS_END
};


}

namespace mozjs
{

const JSClass JsGdiUtils::JsClass = jsClass;
const JSFunctionSpec* JsGdiUtils::JsFunctions = jsFunctions;
const JSPropertySpec* JsGdiUtils::JsProperties = jsProperties;

JsGdiUtils::JsGdiUtils( JSContext* cx )
    : pJsCtx_( cx )
{
}

JsGdiUtils::~JsGdiUtils()
{
}

std::unique_ptr<JsGdiUtils>
JsGdiUtils::CreateNative( JSContext* cx )
{
    return std::unique_ptr<JsGdiUtils>( new JsGdiUtils( cx ) );
}

size_t JsGdiUtils::GetInternalSize()
{
    return 0;
}

std::optional<JSObject*>
JsGdiUtils::CreateImage( uint32_t w, uint32_t h )
{
    std::unique_ptr<Gdiplus::Bitmap> img( new Gdiplus::Bitmap( w, h, PixelFormat32bppPARGB ) );
    if ( !gdi::IsGdiPlusObjectValid( img.get() ) )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Bitmap creation failed" );
        return std::nullopt;
    }

    JS::RootedObject jsObject( pJsCtx_, JsGdiBitmap::CreateJs( pJsCtx_, std::move(img) ) );
    if ( !jsObject )
    {// report in Create
        return std::nullopt;
    }

    return jsObject;
}

std::optional<JSObject*>
JsGdiUtils::Font( const std::wstring& fontName, float pxSize, uint32_t style )
{
    std::unique_ptr<Gdiplus::Font> pGdiFont( new Gdiplus::Font( fontName.c_str(), pxSize, style, Gdiplus::UnitPixel ) );
    if ( !gdi::IsGdiPlusObjectValid( pGdiFont.get() ) )
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
    IF_WINAPI_FAILED_RETURN_WITH_REPORT( pJsCtx_, !!hFont, std::nullopt, CreateFont );
    scope::final_action autoFont( [hFont]()
    {
        DeleteObject( hFont );
    } );

    JS::RootedObject jsObject( pJsCtx_, JsGdiFont::CreateJs( pJsCtx_, std::move(pGdiFont), hFont, true ) );
    if ( !jsObject )
    {// report in Create
        return std::nullopt;
    }

    autoFont.cancel();
    return jsObject;
}

std::optional<JSObject*>
JsGdiUtils::FontWithOpt( size_t optArgCount, const std::wstring& fontName, float pxSize, uint32_t style )
{
    if ( optArgCount > 1 )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: invalid number of optional arguments specified: %d", optArgCount );
        return std::nullopt;
    }

    if ( optArgCount == 1 )
    {
        return Font( fontName, pxSize );
    }

    return Font( fontName, pxSize, style );
}

std::optional<JSObject*> 
JsGdiUtils::Image( const std::wstring& path )
{
    std::unique_ptr<Gdiplus::Bitmap> img = image::LoadImage(path);
    if ( !img )
    {
        return nullptr;
    }

    JS::RootedObject jsObject( pJsCtx_, JsGdiBitmap::CreateJs( pJsCtx_, std::move(img) ) );
    if ( !jsObject )
    {// report in Create
        return std::nullopt;
    }

    return jsObject;
}

std::optional<std::uint32_t> 
JsGdiUtils::LoadImageAsync( uint32_t hWnd, const std::wstring& path )
{
    if ( !hWnd )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Invalid hWnd argument" );
        return std::nullopt;
    }
    // Such cast will work only on x86
    return image::LoadImageAsync( (HWND)hWnd, path );
}

}
