#include <stdafx.h>
#include "gdi_raw_bitmap.h"

#include <js_engine/js_to_native_invoker.h>
#include <utils/gdi_error_helpers.h>
#include <utils/gdi_helpers.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <utils/winapi_error_helpers.h>
#include <utils/scope_helpers.h>

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
    JsGdiRawBitmap::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "GdiRawBitMap",
    kDefaultClassFlags,
    &jsOps
};

const JSFunctionSpec jsFunctions[] = {
    JS_FS_END
};

MJS_DEFINE_JS_FN_FROM_NATIVE( get_Height, JsGdiRawBitmap::get_Height )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Width, JsGdiRawBitmap::get_Width )

const JSPropertySpec jsProperties[] = {
    JS_PSG( "Height", get_Height, kDefaultPropsFlags ),
    JS_PSG( "Width", get_Width, kDefaultPropsFlags ),
    JS_PS_END
};

} // namespace

namespace mozjs
{

const JSClass JsGdiRawBitmap::JsClass = jsClass;
const JSFunctionSpec* JsGdiRawBitmap::JsFunctions = jsFunctions;
const JSPropertySpec* JsGdiRawBitmap::JsProperties = jsProperties;
const JsPrototypeId JsGdiRawBitmap::PrototypeId = JsPrototypeId::GdiRawBitmap;

JsGdiRawBitmap::JsGdiRawBitmap( JSContext* cx,
                                gdi::unique_gdi_ptr<HDC> hDc,
                                gdi::unique_gdi_ptr<HBITMAP> hBmp,
                                uint32_t width,
                                uint32_t height )
    : pJsCtx_( cx )
    , pDc_( std::move( hDc ) )
    , hBmp_( std::move( hBmp ) )
    , autoBmp_( pDc_.get(), hBmp_.get() )
    , width_( width )
    , height_( height )
{
}

JsGdiRawBitmap::~JsGdiRawBitmap()
{
}

std::unique_ptr<JsGdiRawBitmap>
JsGdiRawBitmap::CreateNative( JSContext* cx, Gdiplus::Bitmap* pBmp )
{
    SmpException::ExpectTrue( pBmp, "Internal error: Gdiplus::Bitmap is null" );

    auto pDc = gdi::CreateUniquePtr( CreateCompatibleDC( nullptr ) );
    smp::error::CheckWinApi( !!pDc, "CreateCompatibleDC" );

    auto hBitmap = gdi::CreateHBitmapFromGdiPlusBitmap( *pBmp );
    SmpException::ExpectTrue( !!hBitmap, "Internal error: failed to get HBITMAP from Gdiplus::Bitmap" );

    return std::unique_ptr<JsGdiRawBitmap>(
        new JsGdiRawBitmap( cx, std::move( pDc ), std::move( hBitmap ), pBmp->GetWidth(), pBmp->GetHeight() ) );
}

size_t JsGdiRawBitmap::GetInternalSize( Gdiplus::Bitmap* pBmp )
{ // We generate only PixelFormat32bppPARGB images
    return pBmp->GetWidth() * pBmp->GetHeight() * Gdiplus::GetPixelFormatSize( PixelFormat32bppPARGB ) / 8;
}

__notnull
HDC JsGdiRawBitmap::GetHDC() const
{
    return pDc_.get();
}

std::uint32_t JsGdiRawBitmap::get_Height()
{
    return height_;
}

std::uint32_t JsGdiRawBitmap::get_Width()
{
    return width_;
}

} // namespace mozjs
