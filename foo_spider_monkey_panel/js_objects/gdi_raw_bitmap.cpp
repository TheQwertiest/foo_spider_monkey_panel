#include <stdafx.h>
#include "gdi_raw_bitmap.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/gdi_error_helper.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>

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
    JsFinalizeOp<JsGdiRawBitmap>,
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

MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiRawBitmap, get_Height )
MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiRawBitmap, get_Width )

const JSPropertySpec jsProperties[] = {
    JS_PSG( "Height", get_Height, DefaultPropsFlags() ),
    JS_PSG( "Width",  get_Width, DefaultPropsFlags() ),
    JS_PS_END
};

const JSFunctionSpec jsFunctions[] = {
    JS_FS_END
};

}

namespace mozjs
{


JsGdiRawBitmap::JsGdiRawBitmap( JSContext* cx, Gdiplus::Bitmap* p_bmp )
    : pJsCtx_( cx )
{
    width_ = p_bmp->GetWidth();
    height_ = p_bmp->GetHeight();

    // TODO: move to create
    hDc_ = CreateCompatibleDC( nullptr );
    hBmp_ = helpers::create_hbitmap_from_gdiplus_bitmap( p_bmp );
    hBmpOld_ = SelectBitmap( hDc_, hBmp_ );
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

JSObject* JsGdiRawBitmap::Create( JSContext* cx, Gdiplus::Bitmap* pBmp )
{
    if ( !pBmp )
    {
        JS_ReportErrorASCII( cx, "Internal error: Gdiplus::Bitmap is null" );
        return nullptr;
    }

    JS::RootedObject jsObj( cx,
                            JS_NewObject( cx, &jsClass ) );
    if ( !jsObj )
    {
        return nullptr;
    }

    if ( !JS_DefineFunctions( cx, jsObj, jsFunctions )
         || !JS_DefineProperties(cx, jsObj, jsProperties ) )
    {
        return nullptr;
    }

    JS_SetPrivate( jsObj, new JsGdiRawBitmap( cx, pBmp ) );

    return jsObj;
}

const JSClass& JsGdiRawBitmap::GetClass()
{
    return jsClass;
}

HDC JsGdiRawBitmap::GetHDC() const
{
    return hDc_;
}

std::optional<std::uint32_t> JsGdiRawBitmap::get_Height()
{
    assert( hDc_ );
    return height_;
}

std::optional<std::uint32_t> JsGdiRawBitmap::get_Width()
{
    assert( hDc_ );
    return width_;
}

}
