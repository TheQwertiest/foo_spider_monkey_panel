#include <stdafx.h>

#include "gdi_font.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/gdi_error_helper.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>

#include <js_objects/object_factory.h>


// TODO: add font caching

namespace mozjs
{

const char JsGdiFontInfo::className[] = "GdiFont";

uint32_t JsGdiFontInfo::classFlags = 0;

const JSFunctionSpec JsGdiFontInfo::functions[] = {
    JS_FS_END
};

const JSPropertySpec JsGdiFontInfo::properties[] = {
    JS_PSG( "Height", &get_Height, 0 ),
    JS_PSG( "Name",   &get_Name, 0 ),
    JS_PSG( "Size",   &get_Size, 0 ),
    JS_PSG( "Style",  &get_Style, 0 ),
    JS_PS_END
};

bool JsGdiFontInfo::ValidateCtorArguments( JSContext* cx, Gdiplus::Font* pGdiFont, HFONT hFont, bool isManaged )
{
    if ( !pGdiFont)
    {
        JS_ReportErrorASCII( cx, "Internal error: Gdiplus::Font object is null" );
        return false;
    }

    if ( !hFont )
    {
        JS_ReportErrorASCII( cx, "Internal error: HFONT object is null" );
        return false;
    }

    return true;
}

JsGdiFont::JsGdiFont( JSContext* cx, Gdiplus::Font* pGdiFont, HFONT hFont, bool isManaged )
    : pJsCtx_( cx )
    , isManaged_( isManaged )
    , pGdi_( pGdiFont )
    , hFont_( hFont )
{
}

JsGdiFont::~JsGdiFont()
{
    if ( hFont_ && isManaged_ )
    {
        DeleteFont( hFont_ );
    }    
}

Gdiplus::Font* JsGdiFont::GdiFont() const
{
    assert( pGdi_ );
    return pGdi_.get();
}

HFONT JsGdiFont::HFont() const
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
