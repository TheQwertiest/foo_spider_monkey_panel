#include <stdafx.h>

#include "gdi_utils.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/gdi_bitmap.h>
#include <js_objects/gdi_font.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_hwnd_helpers.h>
#include <js_utils/js_image_helpers.h>
#include <js_utils/js_object_helper.h>
#include <utils/gdi_error_helpers.h>
#include <utils/gdi_helpers.h>
#include <utils/image_helpers.h>

#include <qwr/winapi_error_helpers.h>

using namespace smp;

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
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( CreateImage, JsGdiUtils::CreateImage )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( Font, JsGdiUtils::Font, JsGdiUtils::FontWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( Image, JsGdiUtils::Image )
MJS_DEFINE_JS_FN_FROM_NATIVE( LoadImageAsync, JsGdiUtils::LoadImageAsync )
MJS_DEFINE_JS_FN_FROM_NATIVE( LoadImageAsyncV2, JsGdiUtils::LoadImageAsyncV2 )

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "CreateImage", CreateImage, 2, kDefaultPropsFlags ),
        JS_FN( "Font", Font, 2, kDefaultPropsFlags ),
        JS_FN( "Image", Image, 1, kDefaultPropsFlags ),
        JS_FN( "LoadImageAsync", LoadImageAsync, 2, kDefaultPropsFlags ),
        JS_FN( "LoadImageAsyncV2", LoadImageAsyncV2, 2, kDefaultPropsFlags ),
        JS_FS_END,
    } );

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PS_END,
    } );

} // namespace

namespace mozjs
{

const JSClass JsGdiUtils::JsClass = jsClass;
const JSFunctionSpec* JsGdiUtils::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsGdiUtils::JsProperties = jsProperties.data();

JsGdiUtils::JsGdiUtils( JSContext* cx )
    : pJsCtx_( cx )
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

JSObject* JsGdiUtils::CreateImage( uint32_t w, uint32_t h )
{
    std::unique_ptr<Gdiplus::Bitmap> img( new Gdiplus::Bitmap( w, h, PixelFormat32bppPARGB ) );
    qwr::error::CheckGdiPlusObject( img );

    return JsGdiBitmap::CreateJs( pJsCtx_, std::move( img ) );
}

JSObject* JsGdiUtils::Font( const std::wstring& fontName, uint32_t pxSize, uint32_t style )
{
    return JsGdiFont::Constructor( pJsCtx_, fontName, pxSize, style );
}

JSObject* JsGdiUtils::FontWithOpt( size_t optArgCount, const std::wstring& fontName, uint32_t pxSize, uint32_t style )
{
    switch ( optArgCount )
    {
    case 0:
        return Font( fontName, pxSize, style );
    case 1:
        return Font( fontName, pxSize );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

JSObject* JsGdiUtils::Image( const std::wstring& path )
{
    std::unique_ptr<Gdiplus::Bitmap> img = smp::image::LoadImage( path );
    if ( !img )
    {
        return nullptr;
    }

    return JsGdiBitmap::CreateJs( pJsCtx_, std::move( img ) );
}

std::uint32_t JsGdiUtils::LoadImageAsync( uint32_t hWnd, const std::wstring& path )
{
    (void)hWnd;
    const HWND hPanel = GetPanelHwndForCurrentGlobal( pJsCtx_ );
    qwr::QwrException::ExpectTrue( hPanel, "Method called before fb2k was initialized completely" );

    return smp::image::LoadImageAsync( hPanel, path );
}

JSObject* JsGdiUtils::LoadImageAsyncV2( uint32_t hWnd, const std::wstring& path )
{
    (void)hWnd;
    const HWND hPanel = GetPanelHwndForCurrentGlobal( pJsCtx_ );
    qwr::QwrException::ExpectTrue( hPanel, "Method called before fb2k was initialized completely" );

    return mozjs::image::GetImagePromise( pJsCtx_, hPanel, path );
}

} // namespace mozjs
