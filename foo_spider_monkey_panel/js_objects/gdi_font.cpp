#include <stdafx.h>

#include "gdi_font.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <utils/gdi_error_helpers.h>

#include <qwr/final_action.h>
#include <qwr/winapi_error_helpers.h>

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
    kDefaultClassFlags,
    &jsOps
};

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_Height, JsGdiFont::get_Height )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Name, JsGdiFont::get_Name )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Size, JsGdiFont::get_Size )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Style, JsGdiFont::get_Style )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "Height", get_Height, kDefaultPropsFlags ),
        JS_PSG( "Name", get_Name, kDefaultPropsFlags ),
        JS_PSG( "Size", get_Size, kDefaultPropsFlags ),
        JS_PSG( "Style", get_Style, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( GdiFont_Constructor, JsGdiFont::Constructor, JsGdiFont::ConstructorWithOpt, 1 )

} // namespace

namespace mozjs
{

const JSClass JsGdiFont::JsClass = jsClass;
const JSFunctionSpec* JsGdiFont::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsGdiFont::JsProperties = jsProperties.data();
const JsPrototypeId JsGdiFont::PrototypeId = JsPrototypeId::GdiFont;
const JSNative JsGdiFont::JsConstructor = ::GdiFont_Constructor;

JsGdiFont::JsGdiFont( JSContext* cx, std::unique_ptr<Gdiplus::Font> gdiFont, HFONT hFont, bool isManaged )
    : pJsCtx_( cx )
    , isManaged_( isManaged )
    , pGdi_( std::move( gdiFont ) )
    , hFont_( hFont )
{
    assert( pGdi_.get() );
    assert( hFont_ );
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
    qwr::QwrException::ExpectTrue( !!pGdiFont, "Internal error: Gdiplus::Font object is null" );
    qwr::QwrException::ExpectTrue( hFont, "Internal error: HFONT object is null" );

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

JSObject* JsGdiFont::Constructor( JSContext* cx, const std::wstring& fontName, uint32_t pxSize, uint32_t style )
{
    auto pGdiFont = std::make_unique<Gdiplus::Font>( fontName.c_str(), static_cast<Gdiplus::REAL>( pxSize ), style, Gdiplus::UnitPixel );
    qwr::error::CheckGdiPlusObject( pGdiFont );

    // Generate HFONT
    // The benefit of replacing Gdiplus::Font::GetLogFontW is that you can get it work with CCF/OpenType fonts.
    HFONT hFont = CreateFont(
        -static_cast<int>( pxSize ),
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
    qwr::error::CheckWinApi( !!hFont, "CreateFont" );
    qwr::final_action autoFont( [hFont]() {
        DeleteObject( hFont );
    } );

    JS::RootedObject jsObject( cx, JsGdiFont::CreateJs( cx, std::move( pGdiFont ), hFont, true ) );
    assert( jsObject );

    autoFont.cancel();
    return jsObject;
}

JSObject* JsGdiFont::ConstructorWithOpt( JSContext* cx, size_t optArgCount, const std::wstring& fontName, uint32_t pxSize, uint32_t style )
{
    switch ( optArgCount )
    {
    case 0:
        return Constructor( cx, fontName, pxSize, style );
    case 1:
        return Constructor( cx, fontName, pxSize );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
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
    std::array<wchar_t, LF_FACESIZE> name{};
    Gdiplus::Status gdiRet = pGdi_->GetFamily( &fontFamily );
    qwr::error::CheckGdi( gdiRet, "GetFamily" );

    gdiRet = fontFamily.GetFamilyName( name.data(), LANG_NEUTRAL );
    qwr::error::CheckGdi( gdiRet, "GetFamilyName" );

    return std::wstring( name.data() );
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
