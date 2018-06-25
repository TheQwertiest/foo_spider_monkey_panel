#include <stdafx.h>

#include "gdi_raw_bitmap.h"

#include <js_engine/js_value_converter.h>
#include <js_engine/js_native_invoker.h>
#include <js_engine/js_error_reporter.h>
#include <js_objects/gdi_error.h>
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
    "GdiRawBitMap",
    JSCLASS_HAS_PRIVATE | JSCLASS_FOREGROUND_FINALIZE,
    &jsOps
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiRawBitmap, Height )
MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiRawBitmap, Width )

const JSPropertySpec jsProperties[] = {
    JS_PSG( "Height", Height, 0 ),
    JS_PSG( "Width", Width, 0 ),
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

JSObject* JsGdiRawBitmap::Create( JSContext* cx, Gdiplus::Bitmap* p_bmp )
{
    assert( p_bmp );

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

    JS_SetPrivate( jsObj, new JsGdiRawBitmap( cx, p_bmp ) );

    return jsObj;
}

const JSClass& JsGdiRawBitmap::GetClass()
{
    return jsClass;
}

std::optional<std::uint32_t> JsGdiRawBitmap::Height()
{
    if ( !hDc_ )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: HDC is null" );
        return std::nullopt;
    }

    return height_;
}

std::optional<std::uint32_t> JsGdiRawBitmap::Width()
{
    if ( !hDc_ )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: HDC is null" );
        return std::nullopt;
    }

    return width_;
}

/*


STDMETHODIMP GdiRawBitmap::get__Handle(HDC* p)
{
if (!m_hdc || !p) return E_POINTER;

*p = m_hdc;
return S_OK;
}

*/

}
