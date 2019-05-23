#include <stdafx.h>
#include "gdi_font.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <utils/gdi_error_helpers.h>
#include <utils/scope_helpers.h>
#include <utils/winapi_error_helpers.h>

// TODO: add font caching

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
    JsGdiFont::FinalizeJsObject,
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

const JSFunctionSpec jsFunctions[] = {
    JS_FS_END
};

MJS_DEFINE_JS_FN_FROM_NATIVE( get_Height, JsGdiFont::get_Height )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Name, JsGdiFont::get_Name )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Size, JsGdiFont::get_Size )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Style, JsGdiFont::get_Style )

const JSPropertySpec jsProperties[] = {
    JS_PSG( "Height", get_Height, DefaultPropsFlags() ),
    JS_PSG( "Name", get_Name, DefaultPropsFlags() ),
    JS_PSG( "Size", get_Size, DefaultPropsFlags() ),
    JS_PSG( "Style", get_Style, DefaultPropsFlags() ),
    JS_PS_END
};

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( GdiFont_Constructor, JsGdiFont::Constructor, JsGdiFont::ConstructorWithOpt, 1 )

} // namespace

namespace mozjs
{

const JSClass JsGdiFont::JsClass = jsClass;
const JSFunctionSpec* JsGdiFont::JsFunctions = jsFunctions;
const JSPropertySpec* JsGdiFont::JsProperties = jsProperties;
const JsPrototypeId JsGdiFont::PrototypeId = JsPrototypeId::GdiFont;
const JSNative JsGdiFont::JsConstructor = ::GdiFont_Constructor;

JsGdiFont::JsGdiFont( JSContext* cx, std::unique_ptr<Gdiplus::Font> gdiFont, HFONT hFont, bool isManaged )
    : pJsCtx_( cx )
    , isManaged_( isManaged )
    , hFont_( hFont )
    , pGdi_( std::move( gdiFont ) )
{
}

JsGdiFont::~JsGdiFont()
{
    if ( hFont_ && isManaged_ )
    {
        DeleteFont( hFont_ );
    }
}

std::unique_ptr<JsGdiFont>
JsGdiFont::CreateNative( JSContext* cx, std::unique_ptr<Gdiplus::Font> pGdiFont, HFONT hFont, bool isManaged )
{
    SmpException::ExpectTrue( !!pGdiFont, "Internal error: Gdiplus::Font object is null" );
    SmpException::ExpectTrue( hFont, "Internal error: HFONT object is null" );

    return std::unique_ptr<JsGdiFont>( new JsGdiFont( cx, std::move( pGdiFont ), hFont, isManaged ) );
}

size_t JsGdiFont::GetInternalSize( const std::unique_ptr<Gdiplus::Font>& /*gdiFont*/, HFONT /*hFont*/, bool isManaged )
{
    return sizeof( Gdiplus::Font ) + ( isManaged ? sizeof( LOGFONT ) : 0 );
}

Gdiplus::Font* JsGdiFont::GdiFont() const
{
    return pGdi_.get();
}

HFONT JsGdiFont::GetHFont() const
{
    return hFont_;
}


JSObject* JsGdiFont::Constructor( JSContext* cx, const std::wstring& fontName, float pxSize, uint32_t style )
{
    std::unique_ptr<Gdiplus::Font> pGdiFont( new Gdiplus::Font( fontName.c_str(), pxSize, style, Gdiplus::UnitPixel ) );
    smp::error::CheckGdiPlusObject( pGdiFont );

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
    smp::error::CheckWinApi( !!hFont, "CreateFont" );
    utils::final_action autoFont( [hFont]() {
        DeleteObject( hFont );
    } );

    JS::RootedObject jsObject( cx, JsGdiFont::CreateJs( cx, std::move( pGdiFont ), hFont, true ) );
    assert( jsObject );

    autoFont.cancel();
    return jsObject;
}

JSObject* JsGdiFont::ConstructorWithOpt( JSContext* cx, size_t optArgCount, const std::wstring& fontName, float pxSize, uint32_t style )
{
    switch ( optArgCount )
    {
    case 0:
        return Constructor( cx, fontName, pxSize, style );
    case 1:
        return Constructor( cx, fontName, pxSize );
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

uint32_t JsGdiFont::get_Height() const
{
    Gdiplus::Bitmap img( 1, 1, PixelFormat32bppPARGB );
    Gdiplus::Graphics g( &img );

    return static_cast<uint32_t>( pGdi_->GetHeight( &g ) );
}

std::wstring JsGdiFont::get_Name() const
{
    Gdiplus::FontFamily fontFamily;
    WCHAR name[LF_FACESIZE] = { 0 };
    Gdiplus::Status gdiRet = pGdi_->GetFamily( &fontFamily );
    smp::error::CheckGdi( gdiRet, "GetFamily" );

    gdiRet = fontFamily.GetFamilyName( name, LANG_NEUTRAL );
    smp::error::CheckGdi( gdiRet, "GetFamilyName" );

    return std::wstring( name );
}

float JsGdiFont::get_Size() const
{
    return pGdi_->GetSize();
}

uint32_t JsGdiFont::get_Style() const
{
    return pGdi_->GetStyle();
}

} // namespace mozjs
