#include <stdafx.h>

#include "theme_manager.h"

#include <convert/native_to_js.h>

#include <js_engine/js_to_native_invoker.h>

#include <js_objects/gdi_font.h>
#include <js_objects/gdi_graphics.h>

#include <js_utils/js_error_helper.h>
#include <js_utils/js_hwnd_helpers.h>
#include <js_utils/js_object_helper.h>

#include <utils/colour_helpers.h>
#include <utils/gdi_error_helpers.h>
#include <utils/gdi_helpers.h>

#include <qwr/final_action.h>
#include <qwr/winapi_error_helpers.h>

//#include <atlgdi.h>

using namespace smp;

using smp::gdi::WrapGdiCalls;

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
    JsThemeManager::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "ThemeManager",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( DrawThemeBackground, JsThemeManager::DrawThemeBackground, JsThemeManager::DrawThemeBackgroundWithOpt, 4 )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( DrawThemeText, JsThemeManager::DrawThemeText, JsThemeManager::DrawThemeTextWithOpt, 2 )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetThemeBackgroundContentRect, JsThemeManager::GetThemeBackgroundContentRect )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetThemeBool, JsThemeManager::GetThemeBool )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetThemeColour, JsThemeManager::GetThemeColour )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetThemeEnumValue, JsThemeManager::GetThemeEnumValue )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetThemeFont, JsThemeManager::GetThemeFont )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetThemeFontArgs, JsThemeManager::GetThemeFontArgs )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetThemeInt, JsThemeManager::GetThemeInt )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetThemeIntList, JsThemeManager::GetThemeIntList )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetThemeMargins, JsThemeManager::GetThemeMargins )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetThemeMetric, JsThemeManager::GetThemeMetric )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetThemePosition, JsThemeManager::GetThemePosition )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( GetThemePartSize, JsThemeManager::GetThemePartSize, JsThemeManager::GetThemePartSizeWithOpt, 4 )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetThemePropertyOrigin, JsThemeManager::GetThemePropertyOrigin )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetThemeRect, JsThemeManager::GetThemeRect )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetThemeSysBool, JsThemeManager::GetThemeSysColour )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetThemeSysColour, JsThemeManager::GetThemeSysColour )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetThemeSysFont, JsThemeManager::GetThemeSysFont )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetThemeSysFontArgs, JsThemeManager::GetThemeSysFontArgs )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetThemeSysInt, JsThemeManager::GetThemeSysInt )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetThemeSysSize, JsThemeManager::GetThemeSysSize )
MJS_DEFINE_JS_FN_FROM_NATIVE( IsThemeFontDefined, JsThemeManager::IsThemeFontDefined )
MJS_DEFINE_JS_FN_FROM_NATIVE( IsThemePartDefined, JsThemeManager::IsThemePartDefined )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( SetPartAndStateID, JsThemeManager::SetPartAndStateID, JsThemeManager::SetPartAndStateIDWithOpt, 1 )

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "DrawThemeBackground", DrawThemeBackground, 5, kDefaultPropsFlags ),
        JS_FN( "DrawThemeText", DrawThemeText, 6, kDefaultPropsFlags ),
        JS_FN( "GetThemeBackgroundContentRect", GetThemeBackgroundContentRect, 4, kDefaultPropsFlags ),
        JS_FN( "GetThemeBool", GetThemeBool, 1, kDefaultPropsFlags ),
        JS_FN( "GetThemeColour", GetThemeColour, 1, kDefaultPropsFlags ),
        JS_FN( "GetThemeEnumValue", GetThemeEnumValue, 1, kDefaultPropsFlags ),
        JS_FN( "GetThemeFont", GetThemeFont, 1, kDefaultPropsFlags ),
        JS_FN( "GetThemeFontArgs", GetThemeFontArgs, 1, kDefaultPropsFlags ),
        JS_FN( "GetThemeInt", GetThemeInt, 1, kDefaultPropsFlags ),
        JS_FN( "GetThemeIntList", GetThemeIntList, 1, kDefaultPropsFlags ),
        JS_FN( "GetThemeMargins", GetThemeMargins, 1, kDefaultPropsFlags ),
        JS_FN( "GetThemeMetric", GetThemeMetric, 1, kDefaultPropsFlags ),
        JS_FN( "GetThemePosition", GetThemePosition, 1, kDefaultPropsFlags ),
        JS_FN( "GetThemePartSize", GetThemePartSize, 1, kDefaultPropsFlags ),
        JS_FN( "GetThemePropertyOrigin", GetThemePropertyOrigin, 1, kDefaultPropsFlags ),
        JS_FN( "GetThemeRect", GetThemeRect, 1, kDefaultPropsFlags ),
        JS_FN( "GetThemeSysBool", GetThemeSysBool, 1, kDefaultPropsFlags ),
        JS_FN( "GetThemeSysColour", GetThemeSysColour, 1, kDefaultPropsFlags ),
        JS_FN( "GetThemeSysFont", GetThemeSysFont, 1, kDefaultPropsFlags ),
        JS_FN( "GetThemeSysFontArgs", GetThemeSysFontArgs, 1, kDefaultPropsFlags ),
        JS_FN( "GetThemeSysInt", GetThemeSysInt, 1, kDefaultPropsFlags ),
        JS_FN( "GetThemeSysSize", GetThemeSysSize, 1, kDefaultPropsFlags ),
        JS_FN( "IsThemePartDefined", IsThemeFontDefined, 0, kDefaultPropsFlags ),
        JS_FN( "IsThemePartDefined", IsThemePartDefined, 2, kDefaultPropsFlags ),
        JS_FN( "SetPartAndStateID", SetPartAndStateID, 1, kDefaultPropsFlags ),
        JS_FS_END,
    } );

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PS_END,
    } );

} // namespace

namespace mozjs
{
const JSClass JsThemeManager::JsClass = jsClass;
const JSFunctionSpec* JsThemeManager::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsThemeManager::JsProperties = jsProperties.data();
const JsPrototypeId JsThemeManager::PrototypeId = JsPrototypeId::ThemeManager;

JsThemeManager::JsThemeManager( JSContext* cx, HTHEME hTheme )
    : pJsCtx_( cx )
    , theme( hTheme )
{
}

JsThemeManager::~JsThemeManager()
{
    if ( theme )
        CloseThemeData( theme );
}

bool JsThemeManager::HasThemeData( HWND hwnd, const std::wstring& classId )
{
    // Since CreateNative return nullptr only on error, we need to validate args beforehand
    HTHEME hTheme = OpenThemeData( hwnd, classId.c_str() );

    bool bFound = !!hTheme;

    if ( hTheme )
        CloseThemeData( hTheme );

    return bFound;
}

std::unique_ptr<JsThemeManager>
JsThemeManager::CreateNative( JSContext* cx, HWND hwnd, const std::wstring& classId )
{
    HTHEME hTheme = OpenThemeData( hwnd, classId.c_str() );
    qwr::QwrException::ExpectTrue( hTheme, "Internal error: Failed to get theme data for the provided class list" );

    return std::unique_ptr<JsThemeManager>( new JsThemeManager( cx, hTheme ) );
}

size_t JsThemeManager::GetInternalSize( HWND /* hwnd */, const std::wstring& /* classId */ )
{
    return 0;
}

void JsThemeManager::DrawThemeBackground( JsGdiGraphics* gr,
                                          int32_t rectX, int32_t rectY, uint32_t rectW, uint32_t rectH,
                                          int32_t clipX, int32_t clipY, uint32_t clipW, uint32_t clipH )
{
    qwr::QwrException::ExpectTrue( gr, "gr argument is null" );

    WrapGdiCalls( gr->GetGraphicsObject(), [&]( HDC dc )
    {
        const makeRECT( rect );
        const makeRECT( clip );

        qwr::error::CheckHR( ::DrawThemeBackground
        (
            theme, dc, partID, stateID, &rect,
            IsRectEmpty (&clip) ? nullptr : &clip
        ), "DrawThemeBackground" );
    } );
}

void JsThemeManager::DrawThemeBackgroundWithOpt( size_t optArgCount, JsGdiGraphics* gr,
                                                 int32_t rectX, int32_t rectY, uint32_t rectW, uint32_t rectH,
                                                 int32_t clipX, int32_t clipY, uint32_t clipW, uint32_t clipH )
{
    switch ( optArgCount )
    {
    case 0:
        return DrawThemeBackground( gr, rectX, rectY, rectW, rectH, clipX, clipY, clipW, clipH );
    case 1:
        return DrawThemeBackground( gr, rectX, rectY, rectW, rectH, clipX, clipY, clipW );
    case 2:
        return DrawThemeBackground( gr, rectX, rectY, rectW, rectH, clipX, clipY );
    case 3:
        return DrawThemeBackground( gr, rectX, rectY, rectW, rectH, clipX );
    case 4:
        return DrawThemeBackground( gr, rectX, rectY, rectW, rectH );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsThemeManager::DrawThemeText( JsGdiGraphics* gr, const std::wstring& text,
                                    int32_t rectX, int32_t rectY, uint32_t rectW, uint32_t rectH,
                                    uint32_t format, uint32_t fontprop )
{
    qwr::QwrException::ExpectTrue( gr, "GdiGraphics argument is null" );

    WrapGdiCalls( gr->GetGraphicsObject(), [&]( HDC dc )
    {
        makeRECT( rect );

        DTTOPTS opts{ sizeof( DTTOPTS ), DTT_COMPOSITED };

        format &= ~DT_MODIFYSTRING;

        // TODO: allow more DTTOPS?
        if (fontprop)
        {
            opts.dwFlags |= DTT_FONTPROP;
            opts.iFontPropId = fontprop;
        }

        if ( format & DT_CALCRECT )
        {
            const RECT oldrect = rect;
            RECT calcrect = rect;

            qwr::error::CheckHR( ::GetThemeTextExtent
            (
                theme, dc, partID, stateID, text.c_str(), -1, format, &rect, &calcrect
            ), "GetThemeTextExtent" );

            format &= ~DT_CALCRECT;

            // adjust vertical align
            if ( format & DT_VCENTER )
            {
                rect.top = oldrect.top + ( ( ( oldrect.bottom - oldrect.top ) - ( calcrect.bottom - calcrect.top ) ) >> 1 );
                rect.bottom = rect.top + ( calcrect.bottom - calcrect.top );
            }
            else if ( format & DT_BOTTOM )
            {
                rect.top = oldrect.bottom - ( calcrect.bottom - calcrect.top );
            }
        }

        // fixes default theme font being a pixelfont:
        // DrawThemeText/Ex uses the currently selected DC font if none is defined ih the themepart
        // which defaults the bitmap font "System"
        // so we pre-select a reasonable default to fallback to
        LOGFONTW logfont = {};
        if ( FAILED( ::GetThemeSysFont( theme, fontprop, &logfont ) ) )
            qwr::error::CheckHR( ::GetThemeSysFont( theme, TMT_FIRSTFONT, &logfont ), "GetThemeSysFont" );

        auto selected = SelectObject( dc, ::CreateFontIndirectW( &logfont ) );

        // call DrawThemeTextEx
        qwr::error::CheckHR( ::DrawThemeTextEx
        (
            theme, dc, partID, stateID, const_cast<wchar_t*>( text.c_str() ), text.length(), format, &rect, &opts
        ), "DrawThemeText" );

        // delete temp font, restore previously selected
        DeleteObject ( SelectObject( dc, selected ) );
    } );
}

void JsThemeManager::DrawThemeTextWithOpt( size_t optArgCount, JsGdiGraphics* gr, const std::wstring& text,
                                           int32_t rectX, int32_t rectY, uint32_t rectW, uint32_t rectH,
                                           uint32_t format, uint32_t fontprop )
{
    switch ( optArgCount )
    {
    case 0:
        return DrawThemeText( gr, text, rectX, rectY, rectW, rectH, format, fontprop );
    case 1:
        return DrawThemeText( gr, text, rectX, rectY, rectW, rectH, format );
    case 2:
        return DrawThemeText( gr, text, rectX, rectY, rectW, rectH );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

bool JsThemeManager::GetThemeBool( int32_t propId )
{
    BOOL value;
    qwr::error::CheckHR( ::GetThemeBool( theme, partID, stateID, propId, &value ), "GetThemeBool" );

    return ( value != FALSE );
}

JS::Value JsThemeManager::GetThemeBackgroundContentRect( int32_t rectX, int32_t rectY, uint32_t rectW, uint32_t rectH )
{
    const HWND wnd = ::GetPanelHwndForCurrentGlobal( pJsCtx_ );
    const HDC dc = GetDC( wnd );
    qwr::final_action autoHdcReleaser( [wnd, dc] { ReleaseDC( wnd, dc ); } );

    const makeRECT( rect );

    RECT bgRect;
    qwr::error::CheckHR( ::GetThemeBackgroundContentRect
    (
        theme, dc, partID, stateID, &rect, &bgRect
    ), "GetThemeBackgroundContentRect" );

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue
    (
        pJsCtx_,
        std::vector<int32_t>{ bgRect.top, bgRect.left, bgRect.bottom - bgRect.top, bgRect.right - bgRect.left },
        &jsValue
    );

    return jsValue;
}

uint32_t JsThemeManager::GetThemeColour( int32_t propId )
{
    COLORREF value;
    qwr::error::CheckHR( ::GetThemeColor( theme, partID, stateID, propId, &value ), "GetThemeColour" );

    return smp::colour::ColorrefToArgb( value );
}

int32_t JsThemeManager::GetThemeEnumValue( int32_t propId )
{
    int32_t value;
    qwr::error::CheckHR( ::GetThemeEnumValue( theme, partID, stateID, propId, &value ), "GetThemeEnumValue" );

    return value;
}

JSObject* JsThemeManager::GetThemeFont( int32_t propId )
{
    const HWND wnd = GetPanelHwndForCurrentGlobal( pJsCtx_ );
    const HDC dc = GetDC( wnd );
    qwr::final_action autoHdcReleaser( [wnd, dc] { ReleaseDC( wnd, dc ); } );

    LOGFONTW logfont;
    if ( FAILED( ::GetThemeFont( theme, dc, partID, stateID, propId, &logfont ) ) )
    {
        if ( FAILED( ::GetThemeSysFont( theme, propId, &logfont ) ) )
            qwr::error::CheckHR( ::GetThemeSysFont( theme, TMT_FIRSTFONT, &logfont ), "GetThemeFont" );
    }

    return JsGdiFont::CreateJs( pJsCtx_, logfont );
}

JS::Value JsThemeManager::GetThemeFontArgs( int32_t propId )
{
    const HWND wnd = GetPanelHwndForCurrentGlobal( pJsCtx_ );
    const HDC dc = GetDC( wnd );
    qwr::final_action autoHdcReleaser( [wnd, dc] { ReleaseDC( wnd, dc ); } );

    LOGFONTW value;
    if ( FAILED( ::GetThemeFont( theme, dc, partID, stateID, propId, &value ) ) )
    {
        if ( FAILED( ::GetThemeSysFont( theme, propId, &value ) ) )
            qwr::error::CheckHR( ::GetThemeSysFont( theme, TMT_FIRSTFONT, &value ), "GetThemeFont" );
    }

    return MakeFontArgs( &value );
}

int32_t JsThemeManager::GetThemeInt( int32_t propId )
{
    int32_t value;
    qwr::error::CheckHR( ::GetThemeInt( theme, partID, stateID, propId, &value ), "GetThemeInt" );
    return value;
}

JS::Value JsThemeManager::GetThemeIntList( int32_t propId )
{
    INTLIST value;
    qwr::error::CheckHR( ::GetThemeIntList( theme, partID, stateID, propId, &value ), "GetThemeInt" );

    std::vector<int32_t> intList{ std::begin( value.iValues ), std::end( value.iValues ) };
    intList.resize( value.iValueCount );

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue( pJsCtx_, intList, &jsValue );

    return jsValue;
}

JS::Value JsThemeManager::GetThemeMargins( int32_t propId )
{
    const HWND wnd = ::GetPanelHwndForCurrentGlobal( pJsCtx_ );
    const HDC dc = GetDC( wnd );
    qwr::final_action autoHdcReleaser( [wnd, dc] { ReleaseDC( wnd, dc ); } );

    MARGINS value;
    qwr::error::CheckHR( ::GetThemeMargins( theme, dc, partID, stateID, propId, nullptr, &value ), "GetThemeMargins" );

    // ccw order (tlbr), as in css3
    std::vector<int32_t> margins{ value.cyTopHeight, value.cxLeftWidth, value.cyBottomHeight, value.cxRightWidth };

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue( pJsCtx_, margins, &jsValue );

    return jsValue;
}

int32_t JsThemeManager::GetThemeMetric( int32_t propId )
{
    const HWND wnd = ::GetPanelHwndForCurrentGlobal( pJsCtx_ );
    const HDC dc = GetDC( wnd );
    qwr::final_action autoHdcReleaser( [wnd, dc] { ReleaseDC( wnd, dc ); } );

    int32_t value;
    qwr::error::CheckHR( ::GetThemeMetric( theme, dc, partID, stateID, propId, &value ), "GetThemeMetric" );

    return value;
}

JS::Value JsThemeManager::GetThemePartSize( int32_t themeSize,
                                            int32_t rectX, int32_t rectY, uint32_t rectW, uint32_t rectH )
{
    const HWND wnd = ::GetPanelHwndForCurrentGlobal( pJsCtx_ );
    const HDC dc = GetDC( wnd );
    qwr::final_action autoHdcReleaser( [wnd, dc] { ReleaseDC( wnd, dc ); } );

    const makeRECT( rect );

    SIZE value;
    qwr::error::CheckHR( ::GetThemePartSize(
        theme, dc, partID, stateID, &rect, static_cast<THEMESIZE>( themeSize ), &value
    ), "GetThemePartSize" );

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue( pJsCtx_, std::vector<int32_t>{ value.cx, value.cy }, &jsValue );

    return jsValue;
}

JS::Value JsThemeManager::GetThemePartSizeWithOpt( size_t optArgCount, int32_t themeSize, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
    switch ( optArgCount )
    {
    case 0:
        return GetThemePartSize( themeSize, x, y, w, h );
    case 1:
        return GetThemePartSize( themeSize, x, y, w );
    case 2:
        return GetThemePartSize( themeSize, x, y );
    case 3:
        return GetThemePartSize( themeSize, x );
    case 4:
        return GetThemePartSize( themeSize );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

JS::Value JsThemeManager::GetThemePosition( int32_t propId )
{
    POINT value;
    qwr::error::CheckHR( ::GetThemePosition( theme, partID, stateID, propId, &value ), "GetThemePosition" );

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue( pJsCtx_, std::vector<int32_t>{ value.x, value.y }, &jsValue );

    return jsValue;
}

int32_t JsThemeManager::GetThemePropertyOrigin( int32_t propId )
{
    PROPERTYORIGIN value;
    qwr::error::CheckHR( ::GetThemePropertyOrigin( theme, partID, stateID, propId, &value ), "GetThemePropertyOrigin" );

    return value;
}

JS::Value JsThemeManager::GetThemeRect( int32_t propId )
{
    RECT value;
    qwr::error::CheckHR( ::GetThemeRect( theme, partID, stateID, propId, &value ), "GetThemeRect" );

    std::vector<int32_t> rect{ value.top, value.left, value.bottom - value.top, value.right - value.left };

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue( pJsCtx_, rect, &jsValue );

    return jsValue;
}

BOOL JsThemeManager::GetThemeSysBool( int32_t boolId )
{
    return ::GetThemeSysBool( theme, boolId );
}

uint32_t JsThemeManager::GetThemeSysColour( int32_t colorId )
{
    return smp::colour::ColorrefToArgb( ::GetThemeSysColor( theme, colorId ) );
}

JSObject* JsThemeManager::GetThemeSysFont( int32_t fontId )
{
    LOGFONTW value;
    qwr::error::CheckHR( ::GetThemeSysFont( theme, fontId, &value ), "GetThemeSysFont" );
    return JsGdiFont::CreateJs( pJsCtx_, value );
}

JS::Value JsThemeManager::GetThemeSysFontArgs( int32_t fontId )
{
    LOGFONTW value;
    qwr::error::CheckHR( ::GetThemeSysFont( theme, fontId, &value ), "GetThemeSysFont" );

    return MakeFontArgs( &value );
}

int32_t JsThemeManager::GetThemeSysInt( int32_t intId )
{
    int32_t value;
    qwr::error::CheckHR( ::GetThemeSysInt( theme, intId, &value ), "GetThemeSysInt" );
    return value;
}

int32_t JsThemeManager::GetThemeSysSize( int32_t sizeId )
{
    return ::GetThemeSysSize( theme, sizeId );
}

bool JsThemeManager::IsThemeFontDefined( int32_t propId )
{
    const HWND wnd = GetPanelHwndForCurrentGlobal( pJsCtx_ );
    const HDC dc = GetDC( wnd );
    qwr::final_action autoHdcReleaser( [wnd, dc] { ReleaseDC( wnd, dc ); } );

    LOGFONTW value;
    return !FAILED( ::GetThemeFont( theme, dc, partID, stateID, propId, &value ) );
}

bool JsThemeManager::IsThemePartDefined( int32_t partId, int32_t stateId )
{
    return ::IsThemePartDefined( theme, partId, stateId );
}

void JsThemeManager::SetPartAndStateID( int32_t partId, int32_t stateId )
{
    partID = partId;
    stateID = stateId;
}

void JsThemeManager::SetPartAndStateIDWithOpt( size_t optArgCount, int32_t partId, int32_t stateId )
{
    switch ( optArgCount )
    {
    case 0:
        return SetPartAndStateID( partId, stateId );
    case 1:
        return SetPartAndStateID( partId );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

JS::Value JsThemeManager::MakeFontArgs( LOGFONTW* plf )
{
    JS::RootedObject jsArray( pJsCtx_, JS_NewArrayObject( pJsCtx_, 3 ) );
    smp::JsException::ExpectTrue( jsArray );

    JS::RootedValue jsValue( pJsCtx_ );
    mozjs::convert::to_js::ToValue( pJsCtx_, std::wstring( plf->lfFaceName ), &jsValue );
    smp::JsException::ExpectTrue( JS_SetElement( pJsCtx_, jsArray, 0, jsValue ) );

    mozjs::convert::to_js::ToValue( pJsCtx_, (int32_t)( 0 - plf->lfHeight ), &jsValue );
    smp::JsException::ExpectTrue( JS_SetElement( pJsCtx_, jsArray, 1, jsValue ) );

    uint32_t fontStyle = 0;

    if ( plf->lfWeight >= FW_BOLD )
        fontStyle &= Gdiplus::FontStyle::FontStyleBold;

    if ( plf->lfItalic )
        fontStyle &= Gdiplus::FontStyle::FontStyleItalic;

    if ( plf->lfUnderline )
        fontStyle &= Gdiplus::FontStyle::FontStyleUnderline;

    if ( plf->lfStrikeOut )
        fontStyle &= Gdiplus::FontStyle::FontStyleStrikeout;

    mozjs::convert::to_js::ToValue( pJsCtx_, fontStyle, &jsValue );
    smp::JsException::ExpectTrue( JS_SetElement( pJsCtx_, jsArray, 2, jsValue ) );

    return JS::ObjectValue( *jsArray );
}

} // namespace mozjs
