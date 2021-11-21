#include <stdafx.h>

#include "gdi_font.h"

#include <js_engine/js_to_native_invoker.h>

#include <js_utils/js_error_helper.h>
#include <js_utils/js_hwnd_helpers.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/js_property_helper.h>

#include <utils/gdi_error_helpers.h>
#include <utils/dwrite_renderer.h>

#include <qwr/final_action.h>

#include <qwr/winapi_error_helpers.h>

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

MJS_DEFINE_JS_FN_FROM_NATIVE( get_Name, JsGdiFont::get_Name )
MJS_DEFINE_JS_FN_FROM_NATIVE( set_Name, JsGdiFont::set_Name )

MJS_DEFINE_JS_FN_FROM_NATIVE( get_Height, JsGdiFont::get_Height )
MJS_DEFINE_JS_FN_FROM_NATIVE( set_Height, JsGdiFont::set_Height )

MJS_DEFINE_JS_FN_FROM_NATIVE( get_Size, JsGdiFont::get_Size )
MJS_DEFINE_JS_FN_FROM_NATIVE( set_Size, JsGdiFont::set_Size )

MJS_DEFINE_JS_FN_FROM_NATIVE( get_Style, JsGdiFont::get_Style )
MJS_DEFINE_JS_FN_FROM_NATIVE( set_Style, JsGdiFont::set_Style )

MJS_DEFINE_JS_FN_FROM_NATIVE( get_Weight, JsGdiFont::get_Weight )
MJS_DEFINE_JS_FN_FROM_NATIVE( set_Weight, JsGdiFont::set_Weight )

#if _FONT_DEV_METRICS
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Logfont, JsGdiFont::get_Logfont )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Metrics, JsGdiFont::get_Metrics )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_DesignMetrics, JsGdiFont::get_DesignMetrics )
#endif

#if _FONT_DEV_CACHE
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Cache, JsGdiFont::get_Cache )
#endif

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSGS( "Height", get_Height, set_Height , kDefaultPropsFlags ),
        JS_PSGS( "Name", get_Name, set_Name , kDefaultPropsFlags ),
        JS_PSGS( "Size", get_Size, set_Size , kDefaultPropsFlags ),
        JS_PSGS( "Style", get_Style, set_Style, kDefaultPropsFlags ),
        JS_PSGS( "Weight", get_Weight, set_Weight, kDefaultPropsFlags ),

#if _FONT_DEV_METRICS
        JS_PSG( "Logfont", get_Logfont, kDefaultPropsFlags ),
        JS_PSG( "Metrics", get_Metrics, kDefaultPropsFlags ),
        JS_PSG( "DesignMetrics", get_DesignMetrics, kDefaultPropsFlags ),
#endif

#if _FONT_DEV_CACHE
        JS_PSG( "Cache", get_Cache, kDefaultPropsFlags ),
#endif

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

JsGdiFont::JsGdiFont( JSContext* ctx, const LOGFONTW& font )
    : pJsCtx_( ctx )
    , logfont( font )
{
    Reload();
}

JsGdiFont::~JsGdiFont()
{
}

std::unique_ptr<JsGdiFont>
JsGdiFont::CreateNative( JSContext* ctx, const LOGFONTW& font )
{
    return std::unique_ptr<JsGdiFont>( new JsGdiFont( ctx, font ) );
}

size_t JsGdiFont::GetInternalSize( const LOGFONTW& )
{
    return sizeof( LOGFONTW ) + sizeof( TEXTMETRICW ) + sizeof( fontcache::shared_hfont );
}

JSObject* JsGdiFont::Constructor( JSContext* ctx,
                                  const std::wstring& fontName, int32_t fontSize, uint32_t fontStyle )
{
    LOGFONTW logfont;

    logfont::Make( fontName, fontSize, fontStyle, logfont );

    JS::RootedObject jsObject( ctx, JsGdiFont::CreateJs( ctx, logfont) );
    return jsObject;
}

JSObject* JsGdiFont::ConstructorWithOpt( JSContext* cx, size_t optArgCount,
                                         const std::wstring& fontName, int32_t fontSize, uint32_t fontStyle )
{
    switch ( optArgCount )
    {
    case 0:
        return Constructor( cx, fontName, fontSize, fontStyle );
    case 1:
        return Constructor( cx, fontName, fontSize );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsGdiFont::Reload()
{
    const HWND wnd = GetPanelHwndForCurrentGlobal( pJsCtx_ );
    const HDC dc = GetDC( wnd );
    qwr::final_action autoHdcReleaser( [wnd, dc] { ReleaseDC( wnd, dc ); } );

#if FONT_CACHE_ABSOLUTE_HEIGHT
    logfont::Normalize( dc, logfont );
#endif

    fontcache::shared_hfont hfont = fontcache::Cache( logfont );

    if ( font == hfont )
        return;

    font = hfont;

    // reload metrics
    smp::gdi::ObjectSelector autoFont( dc, HFont() );
    qwr::QwrException::ExpectTrue( GetTextMetricsW( dc, &metric ), "GetTextMetrics" );
}

HFONT JsGdiFont::HFont() const
{
    return font.get();
}

std::wstring JsGdiFont::get_Name() const
{
    return std::wstring( logfont.lfFaceName );
}

void JsGdiFont::set_Name( const std::wstring& fontName )
{
    fontName.copy( logfont.lfFaceName, LF_FACESIZE - 1 );
    Reload();
}

uint32_t JsGdiFont::get_Height() const
{
    return metric.tmHeight;
}

void JsGdiFont::set_Height( uint32_t fontHeight )
{
    logfont.lfHeight = fontHeight;
    Reload();
}

uint32_t JsGdiFont::get_Size() const
{
    return ( metric.tmHeight - metric.tmInternalLeading );
}

void JsGdiFont::set_Size( uint32_t fontSize )
{
    logfont.lfHeight = ( 0 - fontSize );
    Reload();
}

uint32_t JsGdiFont::get_Style() const
{
    uint32_t style = Gdiplus::FontStyleRegular;

    if ( logfont.lfWeight > FW_MEDIUM )
        style |= Gdiplus::FontStyleBold;

    if ( logfont.lfItalic )
        style |= Gdiplus::FontStyleItalic;

    if ( logfont.lfUnderline )
        style |= Gdiplus::FontStyleUnderline;

    if ( logfont.lfStrikeOut )
        style |= Gdiplus::FontStyleStrikeout;

    return style;
}

void JsGdiFont::set_Style( uint32_t fontStyle )
{
    logfont.lfWeight = ( fontStyle & Gdiplus::FontStyleBold )
                           ? std::max<LONG>( FW_BOLD, logfont.lfWeight )
                           : std::min<LONG>( FW_NORMAL, logfont.lfWeight );
    logfont.lfItalic    = !!( fontStyle & Gdiplus::FontStyleItalic );
    logfont.lfUnderline = !!( fontStyle & Gdiplus::FontStyleUnderline );
    logfont.lfStrikeOut = !!( fontStyle & Gdiplus::FontStyleStrikeout );

    Reload();
}

uint32_t JsGdiFont::get_Weight() const
{
    return logfont.lfWeight;
}

void JsGdiFont::set_Weight( uint32_t fontWeight )
{
    logfont.lfWeight = std::clamp<LONG>( fontWeight, 1, 1000 );
    Reload();
}

bool JsGdiFont::get_Italic() const
{
    return logfont.lfItalic;
}

void JsGdiFont::set_Italic( bool italic )
{
    logfont.lfItalic = italic;
    Reload();
}

bool JsGdiFont::get_Underline() const
{
    return logfont.lfUnderline;
}

void JsGdiFont::set_Underline( bool underline )
{
    logfont.lfUnderline = underline;
    Reload();
}

bool JsGdiFont::get_Strikeout() const
{
    return logfont.lfStrikeOut;
}

void JsGdiFont::set_Strikeout( bool strikeout )
{
    logfont.lfStrikeOut = strikeout;
    Reload();
}

#if _FONT_DEV_METRICS
JS::Value JsGdiFont::get_Logfont() const
{
    JS::RootedObject jsObject( pJsCtx_, JS_NewPlainObject( pJsCtx_ ) );

    AddProperty( pJsCtx_, jsObject, "lfHeight",           (int32_t) logfont.lfHeight );
    AddProperty( pJsCtx_, jsObject, "lfWidth",            (int32_t) logfont.lfWidth );
    AddProperty( pJsCtx_, jsObject, "lfEscapement",       (int32_t) logfont.lfEscapement );
    AddProperty( pJsCtx_, jsObject, "lfOrientation",      (int32_t) logfont.lfOrientation );
    AddProperty( pJsCtx_, jsObject, "lfWeight",           (int32_t) logfont.lfWeight );
    AddProperty( pJsCtx_, jsObject, "lfItali",            (int32_t) logfont.lfItalic);
    AddProperty( pJsCtx_, jsObject, "lfUnderline",        (int32_t) logfont.lfUnderline );
    AddProperty( pJsCtx_, jsObject, "lfStrikeOut",        (int32_t) logfont.lfStrikeOut );
    AddProperty( pJsCtx_, jsObject, "lfCharSet",          (int32_t) logfont.lfCharSet );
    AddProperty( pJsCtx_, jsObject, "lfOutPrecision",     (int32_t) logfont.lfOutPrecision );
    AddProperty( pJsCtx_, jsObject, "lfClipPrecision",    (int32_t) logfont.lfClipPrecision );
    AddProperty( pJsCtx_, jsObject, "lfQuality",          (int32_t) logfont.lfQuality );
    AddProperty( pJsCtx_, jsObject, "lfPitchAndFamily",   (int32_t) logfont.lfPitchAndFamily );
    AddProperty( pJsCtx_, jsObject, "lfFaceName",     std::wstring( logfont.lfFaceName ) );

    return JS::ObjectValue( *jsObject );
}

JS::Value JsGdiFont::get_Metrics() const
{
    JS::RootedObject jsObject( pJsCtx_, JS_NewPlainObject( pJsCtx_ ) );

    AddProperty( pJsCtx_, jsObject, "tmHeight",           (int32_t) metric.tmHeight );
    AddProperty( pJsCtx_, jsObject, "tmAscent",           (int32_t) metric.tmAscent );
    AddProperty( pJsCtx_, jsObject, "tmDescent",          (int32_t) metric.tmDescent );
    AddProperty( pJsCtx_, jsObject, "tmInternalLeading",  (int32_t) metric.tmInternalLeading );
    AddProperty( pJsCtx_, jsObject, "tmExternalLeading",  (int32_t) metric.tmExternalLeading );
    AddProperty( pJsCtx_, jsObject, "tmAveCharWidth",     (int32_t) metric.tmAveCharWidth );
    AddProperty( pJsCtx_, jsObject, "tmMaxCharWidth",     (int32_t) metric.tmMaxCharWidth );
    AddProperty( pJsCtx_, jsObject, "tmWeight",           (int32_t) metric.tmWeight );
    AddProperty( pJsCtx_, jsObject, "tmOverhang",         (int32_t) metric.tmOverhang );
    AddProperty( pJsCtx_, jsObject, "tmDigitizedAspectX", (int32_t) metric.tmDigitizedAspectX );
    AddProperty( pJsCtx_, jsObject, "tmDigitizedAspectY", (int32_t) metric.tmDigitizedAspectY );
    AddProperty( pJsCtx_, jsObject, "tmUnderlined",       (int32_t) metric.tmUnderlined );
    AddProperty( pJsCtx_, jsObject, "tmStruckOut",        (int32_t) metric.tmStruckOut );
    AddProperty( pJsCtx_, jsObject, "tmPitchAndFamily",   (int32_t) metric.tmPitchAndFamily );
    AddProperty( pJsCtx_, jsObject, "tmCharSet",          (int32_t) metric.tmCharSet );

    return JS::ObjectValue( *jsObject );
}

JS::Value JsGdiFont::get_DesignMetrics() const
{
    using smp::dwrite::GdiTextRenderer;

    const HWND wnd = ::GetPanelHwndForCurrentGlobal( pJsCtx_ );
    const HDC dc = GetDC( wnd );
    qwr::final_action autoHdcReleaser( [wnd, dc] { ReleaseDC( wnd, dc ); } );
    smp::gdi::ObjectSelector autoFont( dc, HFont() );

    CComPtr<IDWriteFontFace> face;
    qwr::error::CheckHR( GdiTextRenderer::GdiInterop()->CreateFontFaceFromHdc
    (
        dc,
        &face
    ), "CreateFontFaceFromHdc" );

    CComPtr<IDWriteFontFace1> face1;
    qwr::error::CheckHR( face.QueryInterface<IDWriteFontFace1>
    (
        &face1
    ), "IDWriteFontFace::QueryInterface<IDWriteFontFace1>" );

    DWRITE_FONT_METRICS1 metrics1;
    face1->GetMetrics( &metrics1 );

    JS::RootedObject jsObject( pJsCtx_, JS_NewPlainObject( pJsCtx_ ) );

    AddProperty( pJsCtx_, jsObject, "designUnitsPerEm",       (int32_t) metrics1.designUnitsPerEm );
    AddProperty( pJsCtx_, jsObject, "ascent",                 (int32_t) metrics1.ascent );
    AddProperty( pJsCtx_, jsObject, "descent",                (int32_t) metrics1.descent );
    AddProperty( pJsCtx_, jsObject, "lineGap",                (int32_t) metrics1.lineGap );
    AddProperty( pJsCtx_, jsObject, "capHeight",              (int32_t) metrics1.capHeight );
    AddProperty( pJsCtx_, jsObject, "xHeight",                (int32_t) metrics1.xHeight );
    AddProperty( pJsCtx_, jsObject, "underlinePosition",      (int32_t) metrics1.underlinePosition );
    AddProperty( pJsCtx_, jsObject, "underlineThickness",     (int32_t) metrics1.underlineThickness );
    AddProperty( pJsCtx_, jsObject, "strikethroughPosition",  (int32_t) metrics1.strikethroughPosition );
    AddProperty( pJsCtx_, jsObject, "strikethroughThickness", (int32_t) metrics1.strikethroughThickness );
    AddProperty( pJsCtx_, jsObject, "glyphBoxLeft",           (int32_t) metrics1.glyphBoxLeft );
    AddProperty( pJsCtx_, jsObject, "glyphBoxTop",            (int32_t) metrics1.glyphBoxTop );
    AddProperty( pJsCtx_, jsObject, "glyphBoxRight",          (int32_t) metrics1.glyphBoxRight );
    AddProperty( pJsCtx_, jsObject, "glyphBoxBottom",         (int32_t) metrics1.glyphBoxBottom );
    AddProperty( pJsCtx_, jsObject, "subscriptPositionX",     (int32_t) metrics1.subscriptPositionX );
    AddProperty( pJsCtx_, jsObject, "subscriptPositionY",     (int32_t) metrics1.subscriptPositionY );
    AddProperty( pJsCtx_, jsObject, "subscriptSizeX",         (int32_t) metrics1.subscriptSizeX );
    AddProperty( pJsCtx_, jsObject, "subscriptSizeY",         (int32_t) metrics1.subscriptSizeY );
    AddProperty( pJsCtx_, jsObject, "superscriptPositionX",   (int32_t) metrics1.superscriptPositionX );
    AddProperty( pJsCtx_, jsObject, "superscriptPositionY",   (int32_t) metrics1.superscriptPositionY );
    AddProperty( pJsCtx_, jsObject, "superscriptSizeX",       (int32_t) metrics1.superscriptSizeX );
    AddProperty( pJsCtx_, jsObject, "superscriptSizeY",       (int32_t) metrics1.superscriptSizeY );
    AddProperty( pJsCtx_, jsObject, "hasTypographicMetrics",     (bool) metrics1.hasTypographicMetrics );

    return JS::ObjectValue( *jsObject );
}
#endif

#if _FONT_DEV_CACHE
JS::Value JsGdiFont::get_Cache() const
{
    // weight to string
    const static std::map<int, std::string> wts = {
        {    0, "n/a" },
        {  100, "Thin" },
        {  200, "Extra Light" },
        {  300, "Light" },
        {  400, "Regular" },
        {  500, "Medium" },
        {  600, "Semi Bold" },
        {  700, "Bold" },
        {  800, "Extra Bold" },
        {  900, "Black" },
    };

    fontcache::Purge( true );

    JS::RootedObject jsObject = JS::RootedObject( pJsCtx_, JS_NewPlainObject( pJsCtx_ ) );

    fontcache::ForEach( [&]( const std::pair<LOGFONTW, fontcache::weak_hfont>& item )
    {
        std::string hash = fmt::format( "{:#08x}", std::hash<LOGFONTW>{}( item.first ) );
        std::string weight = wts.lower_bound( item.first.lfWeight )->second;

        JS::RootedObject jsKey = JS::RootedObject( pJsCtx_, JS_NewPlainObject( pJsCtx_ ) );

        AddProperty( pJsCtx_, jsKey, "name", std::wstring( item.first.lfFaceName ) );
        AddProperty( pJsCtx_, jsKey, "height", (int32_t)item.first.lfHeight );
        AddProperty( pJsCtx_, jsKey, "weight", fmt::format( "{} ({})", weight, item.first.lfWeight ) );
        AddProperty( pJsCtx_, jsKey, "italic", (bool)item.first.lfItalic );
        AddProperty( pJsCtx_, jsKey, "underline", (bool)item.first.lfUnderline );
        AddProperty( pJsCtx_, jsKey, "strikeout", (bool)item.first.lfStrikeOut );

        JS::RootedObject jsValue = JS::RootedObject( pJsCtx_, JS_NewPlainObject( pJsCtx_ ) );

        HFONT handle = [&] { auto temp = item.second.lock(); return temp.get(); }();

        AddProperty( pJsCtx_, jsValue, "HFONT", fmt::format( "{:#08x}", (size_t)handle ) );
        AddProperty( pJsCtx_, jsValue, "expired", item.second.expired() );
        AddProperty( pJsCtx_, jsValue, "use_count", (int32_t)item.second.use_count() );

        JS::RootedObject jsItem = JS::RootedObject( pJsCtx_, JS_NewPlainObject( pJsCtx_ ) );

        AddProperty( pJsCtx_, jsItem, "Key", JS::HandleObject( jsKey ) );
        AddProperty( pJsCtx_, jsItem, "Value", JS::HandleObject( jsValue ) );

        AddProperty( pJsCtx_, jsObject, hash, JS::HandleObject( jsItem ) );
    } );

    return JS::ObjectValue( *jsObject );
}
#endif

} // namespace mozjs
