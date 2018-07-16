#include <stdafx.h>
#include "gdi_raw_bitmap.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/gdi_error_helper.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
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
    JsGdiRawBitmap::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "GdiRawBitMap",
    DefaultClassFlags(),
    &jsOps
};

const JSFunctionSpec jsFunctions[] = {
    JS_FS_END
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiRawBitmap, get_Height )
MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiRawBitmap, get_Width )

const JSPropertySpec jsProperties[] = {
    JS_PSG( "Height", get_Height, DefaultPropsFlags() ),
    JS_PSG( "Width",  get_Width, DefaultPropsFlags() ),
    JS_PS_END
};

}

namespace mozjs
{

const JSClass JsGdiRawBitmap::JsClass = jsClass;
const JSFunctionSpec* JsGdiRawBitmap::JsFunctions = jsFunctions;
const JSPropertySpec* JsGdiRawBitmap::JsProperties = jsProperties;
const JsPrototypeId JsGdiRawBitmap::PrototypeId = JsPrototypeId::GdiRawBitmap;

JsGdiRawBitmap::JsGdiRawBitmap( JSContext* cx, HDC hDc, HBITMAP hBmp, uint32_t width, uint32_t height )
    : pJsCtx_( cx )
    , hDc_(hDc)
    , hBmp_(hBmp)
    , width_(width)
    , height_(height)
{
    hBmpOld_ = SelectBitmap( hDc, hBmp );
}

JsGdiRawBitmap::~JsGdiRawBitmap()
{
    if ( hDc_ )
    {
        SelectBitmap( hDc_, hBmpOld_ );
        DeleteDC( hDc_ );
        hDc_ = nullptr;
    }

    if ( hBmp_ )
    {
        DeleteBitmap( hBmp_ );
        hBmp_ = nullptr;
    }
}


std::unique_ptr<JsGdiRawBitmap>
JsGdiRawBitmap::CreateNative( JSContext* cx, Gdiplus::Bitmap* pBmp )
{
    if ( !pBmp )
    {
        JS_ReportErrorUTF8( cx, "Internal error: Gdiplus::Bitmap is null" );
        return nullptr;
    }

    HDC hDc = CreateCompatibleDC( nullptr );
    IF_WINAPI_FAILED_RETURN_WITH_REPORT( cx, !!hDc, nullptr, CreateCompatibleDC );

    HBITMAP hBmp = helpers::create_hbitmap_from_gdiplus_bitmap( pBmp );
    if ( !hBmp )
    {
        DeleteDC( hDc );

        JS_ReportErrorUTF8( cx, "Internal error: failed to get HBITMAP from Gdiplus::Bitmap" );
        return nullptr;
    }

    return std::unique_ptr<JsGdiRawBitmap>( new JsGdiRawBitmap( cx, hDc, hBmp, pBmp->GetWidth(), pBmp->GetHeight() ) );
}

size_t JsGdiRawBitmap::GetInternalSize( Gdiplus::Bitmap* pBmp )
{// We generate only PixelFormat32bppPARGB images
    return pBmp->GetWidth()*pBmp->GetHeight()*Gdiplus::GetPixelFormatSize( PixelFormat32bppPARGB );
}

HDC JsGdiRawBitmap::GetHDC() const
{
    return hDc_;
}

std::optional<std::uint32_t> JsGdiRawBitmap::get_Height()
{
    return height_;
}

std::optional<std::uint32_t> JsGdiRawBitmap::get_Width()
{
    return width_;
}

}
