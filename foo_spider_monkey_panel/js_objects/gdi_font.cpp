#include <stdafx.h>

#include "gdi_font.h"

#include <js_engine/js_value_converter.h>
#include <js_engine/js_native_invoker.h>
#include <js_engine/js_error_reporter.h>
#include <js_objects/gdi_error.h>
#include <js_utils/js_utils.h>


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
    "GdiFont",
    JSCLASS_HAS_PRIVATE | JSCLASS_FOREGROUND_FINALIZE,
    &jsOps
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiFont, Height )
MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiFont, Name )
MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiFont, Size )
MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiFont, Style )

const JSPropertySpec jsProperties[] = {
    JS_PSG( "Height", Height, 0 ),
    JS_PSG( "Name", Name, 0 ),
    JS_PSG( "Size", Size, 0 ),
    JS_PSG( "Style", Style, 0 ),
    JS_PS_END
};


const JSFunctionSpec jsFunctions[] = {
    JS_FS_END
};

}

namespace mozjs
{


JsGdiFont::JsGdiFont( JSContext* cx, Gdiplus::Font* pGdiFont, HFONT hFont )
    : pJsCtx_( cx )
    , pGdi_( pGdiFont )
    , hFont_( hFont )
{
}


JsGdiFont::~JsGdiFont()
{
}
// TODO: implement isManaged
JSObject* JsGdiFont::Create( JSContext* cx, Gdiplus::Font* pGdiFont, HFONT hFont, bool isManaged )
{
    assert( pGdiFont );

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

    JS_SetPrivate( jsObj, new JsGdiFont( cx, pGdiFont, hFont ) );

    return jsObj;
}

const JSClass& JsGdiFont::GetClass()
{
    return jsClass;
}

Gdiplus::Font* JsGdiFont::GdiFont() const
{
    return pGdi_.get();
}

HFONT JsGdiFont::HFont() const
{
    return hFont_;
}

std::optional<uint32_t>
JsGdiFont::Height() const
{
    if ( !pGdi_ )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: Gdiplus::Font object is null" );
        return std::nullopt;
    }

    Gdiplus::Bitmap img( 1, 1, PixelFormat32bppPARGB );
    Gdiplus::Graphics g( &img );

    return static_cast<uint32_t>(pGdi_->GetHeight( &g ));
}

std::optional<std::wstring>
JsGdiFont::Name() const
{
    if ( !pGdi_ )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: Gdiplus::Font object is null" );
        return std::nullopt;
    }

    Gdiplus::FontFamily fontFamily;
    WCHAR name[LF_FACESIZE] = { 0 };
    Gdiplus::Status gdiRet = pGdi_->GetFamily( &fontFamily );
    IF_GDI_FAILED_RETURN_WITH_REPORT( pJsCtx_, gdiRet, std::nullopt, GetFamily );

    gdiRet = fontFamily.GetFamilyName( name, LANG_NEUTRAL );
    IF_GDI_FAILED_RETURN_WITH_REPORT( pJsCtx_, gdiRet, std::nullopt, GetFamilyName );
    
    return std::wstring(name);
}

std::optional<float>
JsGdiFont::Size() const
{
    if ( !pGdi_ )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: Gdiplus::Font object is null" );
        return std::nullopt;
    }

    return pGdi_->GetSize();
}

std::optional<uint32_t>
JsGdiFont::Style() const
{
    if ( !pGdi_ )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: Gdiplus::Font object is null" );
        return std::nullopt;
    }

    return pGdi_->GetStyle();
}

}
