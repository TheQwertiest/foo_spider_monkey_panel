#include <stdafx.h>

#include "gdi_font.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/gdi_error_helper.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>


// TODO: add font caching

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
    JsFinalizeOp<JsGdiFont>,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "GdiFont",
    DefaultClassFlags(),
    &jsOps
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiFont, get_Height )
MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiFont, get_Name )
MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiFont, get_Size )
MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiFont, get_Style )

const JSPropertySpec jsProperties[] = {
    JS_PSG( "Height", get_Height, DefaultPropsFlags() ),
    JS_PSG( "Name",   get_Name, DefaultPropsFlags() ),
    JS_PSG( "Size",   get_Size, DefaultPropsFlags() ),
    JS_PSG( "Style",  get_Style, DefaultPropsFlags() ),
    JS_PS_END
};


const JSFunctionSpec jsFunctions[] = {
    JS_FS_END
};

}

namespace mozjs
{

const JSClass JsGdiFont::JsClass = jsClass;
const JSFunctionSpec* JsGdiFont::JsFunctions = jsFunctions;
const JSPropertySpec* JsGdiFont::JsProperties = jsProperties;
const JsPrototypeId JsGdiFont::PrototypeId = JsPrototypeId::GdiFont;

JsGdiFont::JsGdiFont( JSContext* cx, std::unique_ptr<Gdiplus::Font> gdiFont, HFONT hFont, bool isManaged )
    : pJsCtx_( cx )
    , isManaged_( isManaged )
    , hFont_( hFont )
{
    pGdi_.swap(gdiFont);
}

JsGdiFont::~JsGdiFont()
{
    if ( hFont_ && isManaged_ )
    {
        DeleteFont( hFont_ );
    }    
}


std::unique_ptr<JsGdiFont> JsGdiFont::CreateNative( JSContext* cx, std::unique_ptr<Gdiplus::Font> pGdiFont, HFONT hFont, bool isManaged )
{
    if ( !pGdiFont )
    {
        JS_ReportErrorUTF8( cx, "Internal error: Gdiplus::Font object is null" );
        return nullptr;
    }

    if ( !hFont )
    {
        JS_ReportErrorUTF8( cx, "Internal error: HFONT object is null" );
        return nullptr;
    }

    return std::unique_ptr<JsGdiFont>( new JsGdiFont(cx, std::move( pGdiFont ), hFont, isManaged ));
}

Gdiplus::Font* JsGdiFont::GdiFont() const
{
    assert( pGdi_ );
    return pGdi_.get();
}

HFONT JsGdiFont::GetHFont() const
{
    assert( hFont_ );
    return hFont_;
}

std::optional<uint32_t>
JsGdiFont::get_Height() const
{
    assert( pGdi_ );

    Gdiplus::Bitmap img( 1, 1, PixelFormat32bppPARGB );
    Gdiplus::Graphics g( &img );

    return static_cast<uint32_t>(pGdi_->GetHeight( &g ));
}

std::optional<std::wstring>
JsGdiFont::get_Name() const
{
    assert( pGdi_ );

    Gdiplus::FontFamily fontFamily;
    WCHAR name[LF_FACESIZE] = { 0 };
    Gdiplus::Status gdiRet = pGdi_->GetFamily( &fontFamily );
    IF_GDI_FAILED_RETURN_WITH_REPORT( pJsCtx_, gdiRet, std::nullopt, GetFamily );

    gdiRet = fontFamily.GetFamilyName( name, LANG_NEUTRAL );
    IF_GDI_FAILED_RETURN_WITH_REPORT( pJsCtx_, gdiRet, std::nullopt, GetFamilyName );
    
    return std::wstring(name);
}

std::optional<float>
JsGdiFont::get_Size() const
{
    assert( pGdi_ );
    return pGdi_->GetSize();
}

std::optional<uint32_t>
JsGdiFont::get_Style() const
{
    assert( pGdi_ );
    return pGdi_->GetStyle();
}

}
