#include <stdafx.h>
#include <variant>

#include "theme_manager.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/gdi_graphics.h>
#include <js_objects/gdi_font.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/js_hwnd_helpers.h>
#include <utils/colour_helpers.h>
#include <utils/gdi_helpers.h>

#include <qwr/final_action.h>
#include <qwr/winapi_error_helpers.h>
#include <convert/native_to_js.h>
#include <utils/gdi_error_helpers.h>

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
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( DrawThemeText, JsThemeManager::DrawThemeText, JsThemeManager::DrawThemeTextWithOpt, 1 )
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
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( GetThemePartSize, JsThemeManager::GetThemePartSize, JsThemeManager::GetThemePartSizeWithOpt, 5 )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetThemePropertyOrigin, JsThemeManager::GetThemePropertyOrigin )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetThemeRect, JsThemeManager::GetThemeRect )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetThemeSysColour, JsThemeManager::GetThemeSysColour )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetThemeSysFont, JsThemeManager::GetThemeSysFont )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetThemeSysFontArgs, JsThemeManager::GetThemeSysFontArgs )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetThemeSysInt, JsThemeManager::GetThemeSysInt )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetThemeSysSize, JsThemeManager::GetThemeSysSize )
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
        JS_FN( "GetThemeSysColour", GetThemeSysColour, 1, kDefaultPropsFlags ),
        JS_FN( "GetThemeSysFont", GetThemeSysFont, 1, kDefaultPropsFlags ),
        JS_FN( "GetThemeSysFontArgs", GetThemeSysFontArgs, 1, kDefaultPropsFlags ),
        JS_FN( "GetThemeSysInt", GetThemeSysInt, 1, kDefaultPropsFlags ),
        JS_FN( "GetThemeSysSize", GetThemeSysSize, 1, kDefaultPropsFlags ),
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
    , hTheme_( hTheme )
{
}

JsThemeManager::~JsThemeManager()
{
    if ( hTheme_ )
    {
        CloseThemeData( hTheme_ );
    }
}

bool JsThemeManager::HasThemeData( HWND hwnd, const std::wstring& classId )
{ // Since CreateNative return nullptr only on error, we need to validate args beforehand
    HTHEME hTheme = OpenThemeData( hwnd, classId.c_str() );
    bool bFound = !!hTheme;
    if ( hTheme )
    {
        CloseThemeData( hTheme );
    }

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
                                          int32_t x, int32_t y, uint32_t w, uint32_t h,
                                          int32_t clip_x, int32_t clip_y, uint32_t clip_w, uint32_t clip_h )
{
    qwr::QwrException::ExpectTrue( gr, "gr argument is null" );

    Gdiplus::Graphics* graphics = gr->GetGraphicsObject();
    assert( graphics );

    // get current clip region and transform
    // we need to do this before getting the dc
    Gdiplus::Region region;
    qwr::error::CheckGdi( graphics->GetClip( &region ), "GetClip" );
    HRGN hrgn{ region.GetHRGN( graphics ) };

    Gdiplus::Matrix matrix;
    qwr::error::CheckGdi( graphics->GetTransform( &matrix ), "GetTransform" );

    XFORM xform;
    qwr::error::CheckGdi( matrix.GetElements( (Gdiplus::REAL*)( &xform ) ), "GetElements" );

    // get the device context
    const HDC dc = graphics->GetHDC();
    qwr::final_action autoReleaseDc( [graphics, dc]() { graphics->ReleaseHDC( dc ); } );

    // set clip and transform
    HRGN oldhrgn = ::CreateRectRgn( 0, 0, 0, 0 ); // dummy region for getting the current clip region into
    qwr::error::CheckWinApi( -1 != ::GetClipRgn( dc, oldhrgn ), "GetClipRgn" );
    qwr::final_action autoReleaseRegion( [dc, oldhrgn]() { ::SelectClipRgn( dc, oldhrgn ); ::DeleteObject( oldhrgn ); } );

    qwr::error::CheckWinApi( ERROR != ::SelectClipRgn( dc, hrgn ), "SelectClipRgn" );
    ::DeleteObject( hrgn ); // see remarks at https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-selectcliprgn

    XFORM oldxform;
    qwr::error::CheckWinApi( ::GetWorldTransform( dc, &oldxform ), "GetWorldTransform" );
    qwr::error::CheckWinApi( ::SetWorldTransform( dc, &xform ), "SetWorldTransform" );
    qwr::final_action autoResetTansform( [dc, oldxform]() { ::SetWorldTransform( dc, &oldxform ); } );

    const RECT rect{ x, y, static_cast<LONG>( x + w ), static_cast<LONG> (y + h) };
    const RECT clip{ clip_x, clip_y, static_cast<LONG>( clip_x + clip_w ), static_cast<LONG>( clip_y + clip_h ) };
    LPCRECT pclip = !( clip_x || clip_y || clip_w || clip_h ) ? nullptr : &clip;

    qwr::error::CheckHR( ::DrawThemeBackground( hTheme_, dc, partId_, stateId_, &rect, pclip ), "DrawThemeBackground" );
}

void JsThemeManager::DrawThemeBackgroundWithOpt( size_t optArgCount, JsGdiGraphics* gr,
                                                 int32_t x, int32_t y, uint32_t w, uint32_t h,
                                                 int32_t clip_x, int32_t clip_y, uint32_t clip_w, uint32_t clip_h )
{
    switch ( optArgCount )
    {
    case 0:
        return DrawThemeBackground( gr, x, y, w, h, clip_x, clip_y, clip_w, clip_h );
    case 1:
        return DrawThemeBackground( gr, x, y, w, h, clip_x, clip_y, clip_w );
    case 2:
        return DrawThemeBackground( gr, x, y, w, h, clip_x, clip_y );
    case 3:
        return DrawThemeBackground( gr, x, y, w, h, clip_x );
    case 4:
        return DrawThemeBackground( gr, x, y, w, h );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsThemeManager::DrawThemeText( JsGdiGraphics* gr,
                                    const std::wstring& str,
                                    int32_t x, int32_t y, uint32_t w, uint32_t h,
                                    uint32_t format )
{
    qwr::QwrException::ExpectTrue( gr, "gr argument is null" );

    Gdiplus::Graphics* graphics = gr->GetGraphicsObject();
    assert( graphics );

    // get current clip region and transform
    // we need to do this before getting the dc
    Gdiplus::Region region;
    qwr::error::CheckGdi( graphics->GetClip( &region ), "GetClip" );
    HRGN hrgn{ region.GetHRGN( graphics ) };

    Gdiplus::Matrix matrix;
    qwr::error::CheckGdi( graphics->GetTransform( &matrix ), "GetTransform" );

    XFORM xform;
    qwr::error::CheckGdi( matrix.GetElements( (Gdiplus::REAL*)( &xform ) ), "GetElements" );

    // get the device context
    const HDC dc = graphics->GetHDC();
    qwr::final_action autoReleaseDc( [graphics, dc]() { graphics->ReleaseHDC( dc ); } );

    // set clip and transform
    HRGN oldhrgn = ::CreateRectRgn( 0, 0, 0, 0 ); // dummy region for getting the current clip region into
    qwr::error::CheckWinApi( -1 != ::GetClipRgn( dc, oldhrgn ), "GetClipRgn" );
    qwr::final_action autoReleaseRegion( [dc, oldhrgn]() { ::SelectClipRgn( dc, oldhrgn ); ::DeleteObject( oldhrgn ); } );

    qwr::error::CheckWinApi( ERROR != ::SelectClipRgn( dc, hrgn ), "SelectClipRgn" );
    ::DeleteObject( hrgn ); // see remarks at https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-selectcliprgn

    XFORM oldxform;
    qwr::error::CheckWinApi( ::GetWorldTransform( dc, &oldxform ), "GetWorldTransform" );
    qwr::error::CheckWinApi( ::SetWorldTransform( dc, &xform ), "SetWorldTransform" );
    qwr::final_action autoResetTansform( [dc, oldxform]() { ::SetWorldTransform( dc, &oldxform ); } );

    RECT rect{ x, y, static_cast<LONG>( x + w ), static_cast<LONG>( y + h ) };

    const DTTOPTS opts{ sizeof( DTTOPTS ), 0 };

    format &= ~DT_MODIFYSTRING;

    if ( format & DT_CALCRECT )
    {
        const RECT oldrect = rect;
        RECT calcrect = rect;

        qwr::error::CheckHR( ::DrawThemeTextEx( hTheme_, dc, partId_, stateId_, str.c_str(), -1, format, &rect, &opts ), "DrawThemeTextEx" );

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

    qwr::error::CheckHR( ::DrawThemeTextEx( hTheme_, dc, partId_, stateId_, const_cast<wchar_t*>( str.c_str() ), -1, format, &rect, &opts ), "DrawThemeTextEx" );
}

void JsThemeManager::DrawThemeTextWithOpt( size_t optArgCount, JsGdiGraphics* gr,
                                           const std::wstring& str,
                                           int32_t x, int32_t y, uint32_t w, uint32_t h,
                                           uint32_t format )
{
    switch ( optArgCount )
    {
    case 0:
        return DrawThemeText( gr, str, x, y, w, h, format );
    case 1:
        return DrawThemeText( gr, str, x, y, w, h );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

bool JsThemeManager::GetThemeBool( int32_t propId )
{
    BOOL value;
    qwr::error::CheckHR( ::GetThemeBool( hTheme_, partId_, stateId_, propId, &value ), "GetThemeBool" );

    return ( value != FALSE );
}

JS::Value JsThemeManager::GetThemeBackgroundContentRect( int32_t x, int32_t y, uint32_t w, uint32_t h )
{
    const HWND wnd = ::GetPanelHwndForCurrentGlobal( pJsCtx_ );
    const HDC dc = GetDC( wnd );
    qwr::final_action autoHdcReleaser( [wnd, dc]() { ReleaseDC( wnd, dc ); } );

    const RECT rect{ x, y, static_cast<LONG>( x + w ), static_cast<LONG>( y + h ) };

    RECT value;
    qwr::error::CheckHR( ::GetThemeBackgroundContentRect( hTheme_, dc, partId_, stateId_, &rect, &value ), "GetThemeBackgroundContentRect" );

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue( pJsCtx_,
                                  std::vector<int32_t>{ value.top, value.left, value.bottom - value.top, value.right - value.left },
                                  &jsValue );

    return jsValue;
}

uint32_t JsThemeManager::GetThemeColour( int32_t propId )
{
    COLORREF value;
    qwr::error::CheckHR( ::GetThemeColor( hTheme_, partId_, stateId_, propId, &value ), "GetThemeColour" );

    return smp::colour::ColorrefToArgb( value );
}

int32_t JsThemeManager::GetThemeEnumValue( int32_t propId )
{
    int32_t value;
    qwr::error::CheckHR( ::GetThemeEnumValue( hTheme_, partId_, stateId_, propId, &value ), "GetThemeEnumValue" );

    return value;
}

JSObject* JsThemeManager::GetThemeFont( int32_t propId )
{
    const HWND wnd = ::GetPanelHwndForCurrentGlobal( pJsCtx_ );
    const HDC dc = GetDC( wnd );
    qwr::final_action autoHdcReleaser( [wnd, dc]() { ReleaseDC( wnd, dc ); } );

    LOGFONTW value;
    qwr::error::CheckHR( ::GetThemeFont( hTheme_, dc, partId_, stateId_, propId, &value ), "GetThemeFont" );

    return MakeFont( dc, &value );
}

JS::Value JsThemeManager::GetThemeFontArgs( int32_t propId )
{
    const HWND wnd = ::GetPanelHwndForCurrentGlobal( pJsCtx_ );
    const HDC dc = GetDC( wnd );
    qwr::final_action autoHdcReleaser( [wnd, dc]() { ReleaseDC( wnd, dc ); } );

    LOGFONTW value;
    qwr::error::CheckHR( ::GetThemeFont( hTheme_, dc, partId_, stateId_, propId, &value ), "GetThemeFont" );

    return MakeFontArgs( &value );
}

int32_t JsThemeManager::GetThemeInt( int32_t propId )
{
    int32_t value;
    qwr::error::CheckHR( ::GetThemeInt( hTheme_, partId_, stateId_, propId, &value ), "GetThemeInt" );

    return value;
}

JS::Value JsThemeManager::GetThemeIntList( int32_t propId )
{
    INTLIST value;
    qwr::error::CheckHR( ::GetThemeIntList( hTheme_, partId_, stateId_, propId, &value ), "GetThemeInt" );

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
    qwr::final_action autoHdcReleaser( [wnd, dc]() { ReleaseDC( wnd, dc ); } );

    MARGINS value;
    qwr::error::CheckHR( ::GetThemeMargins( hTheme_, dc, partId_, stateId_, propId, nullptr, &value ), "GetThemeMargins" );

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
    qwr::final_action autoHdcReleaser( [wnd, dc]() { ReleaseDC( wnd, dc ); } );

    int32_t value;
    qwr::error::CheckHR( ::GetThemeMetric( hTheme_, dc, partId_, stateId_, propId, &value ), "GetThemeMetric" );

    return value;
}

JS::Value JsThemeManager::GetThemePartSize( int32_t themeSize, JsGdiGraphics* gr, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
    const HWND wnd = ::GetPanelHwndForCurrentGlobal( pJsCtx_ );
    const HDC dc = GetDC( wnd );
    qwr::final_action autoHdcReleaser( [wnd, dc]() { ReleaseDC( wnd, dc ); } );

    const RECT rect{ x, y, static_cast<LONG>( x + w ), static_cast<LONG>( y + h ) };

    SIZE value;
    qwr::error::CheckHR( ::GetThemePartSize( hTheme_, dc, partId_, stateId_, &rect, static_cast<THEMESIZE>( themeSize ), &value ), "GetThemePartSize" );

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue( pJsCtx_, std::vector<int32_t>{ value.cx, value.cy }, &jsValue );

    return jsValue;
}

JS::Value JsThemeManager::GetThemePartSizeWithOpt( size_t optArgCount, int32_t themeSize, JsGdiGraphics* gr, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
    switch ( optArgCount )
    {
    case 0:
        return GetThemePartSize( themeSize, gr, x, y, w, h );
    case 1:
        return GetThemePartSize( themeSize, gr, x, y, w );
    case 2:
        return GetThemePartSize( themeSize, gr, x, y );
    case 3:
        return GetThemePartSize( themeSize, gr, x );
    case 4:
        return GetThemePartSize( themeSize, gr );
    case 5:
        return GetThemePartSize( themeSize );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

JS::Value JsThemeManager::GetThemePosition( int32_t propId )
{
    POINT value;
    qwr::error::CheckHR( ::GetThemePosition( hTheme_, partId_, stateId_, propId, &value ), "GetThemePosition" );

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue( pJsCtx_, std::vector<int32_t>{ value.x, value.y }, &jsValue );

    return jsValue;
}

int32_t JsThemeManager::GetThemePropertyOrigin( int32_t propId )
{
    PROPERTYORIGIN value;
    qwr::error::CheckHR( ::GetThemePropertyOrigin( hTheme_, partId_, stateId_, propId, &value ), "GetThemePropertyOrigin" );

    return value;
}

JS::Value JsThemeManager::GetThemeRect( int32_t propId )
{
    RECT value;
    qwr::error::CheckHR( ::GetThemeRect( hTheme_, partId_, stateId_, propId, &value ), "GetThemeRect" );

    std::vector<int32_t> rect{ value.top, value.left, value.bottom - value.top, value.right - value.left };

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue( pJsCtx_, rect,  &jsValue );

    return jsValue;
}

uint32_t JsThemeManager::GetThemeSysColour( int32_t propId )
{
    return smp::colour::ColorrefToArgb( ::GetThemeSysColor( hTheme_, propId ) );
}

JSObject* JsThemeManager::GetThemeSysFont( int32_t propId )
{
    const HWND wnd = ::GetPanelHwndForCurrentGlobal( pJsCtx_ );
    const HDC dc = GetDC( wnd );
    qwr::final_action autoHdcReleaser( [wnd, dc]() { ReleaseDC( wnd, dc ); } );

    LOGFONTW value;
    qwr::error::CheckHR( ::GetThemeSysFont( hTheme_, propId, &value ), "GetThemeSysFont" );

    return MakeFont( dc, &value );
}

JS::Value JsThemeManager::GetThemeSysFontArgs( int32_t propId )
{
    LOGFONTW value;
    qwr::error::CheckHR( ::GetThemeSysFont( hTheme_, propId, &value ), "GetThemeSysFont" );

    return MakeFontArgs( &value );
}

int32_t JsThemeManager::GetThemeSysInt( int32_t propId )
{
    int32_t value;
    qwr::error::CheckHR( ::GetThemeSysInt( hTheme_, propId, &value ), "GetThemeSysInt" );
    return value;
}

int32_t JsThemeManager::GetThemeSysSize( int32_t propId )
{
    return ::GetThemeSysSize( hTheme_, propId );
}

bool JsThemeManager::IsThemePartDefined( int32_t partId, int32_t stateId )
{
    return ::IsThemePartDefined( hTheme_, partId, stateId );
}

void JsThemeManager::SetPartAndStateID( int32_t partId, int32_t stateId )
{
    partId_ = partId;
    stateId_ = stateId;
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

JSObject* JsThemeManager::MakeFont( HDC dc, LOGFONTW* plf )
{
    HFONT hFont = ::CreateFontIndirectW( plf );

    std::unique_ptr<Gdiplus::Font> pGdiFont( new Gdiplus::Font( dc, hFont ) );
    if ( !gdi::IsGdiPlusObjectValid( pGdiFont ) )
    { // Not an error: font not found
        return nullptr;
    }

    return JsGdiFont::CreateJs( pJsCtx_, std::move( pGdiFont ), hFont, false );
}

JS::Value JsThemeManager::MakeFontArgs( LOGFONTW* plf )
{
    JS::RootedObject jsArray( pJsCtx_, JS_NewArrayObject( pJsCtx_, 3 ) );
    smp::JsException::ExpectTrue( jsArray );

    JS::RootedValue jsValue( pJsCtx_ );
    size_t i = 0;
    mozjs::convert::to_js::ToValue( pJsCtx_, std::wstring( plf->lfFaceName ), &jsValue );
    if ( !JS_SetElement( pJsCtx_, jsArray, i++, jsValue ) )
    {
        throw smp::JsException();
    }

    mozjs::convert::to_js::ToValue( pJsCtx_, (int32_t)(0 - plf->lfHeight), &jsValue );
    if ( !JS_SetElement( pJsCtx_, jsArray, i++, jsValue ) )
    {
        throw smp::JsException();
    }

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
    if ( !JS_SetElement( pJsCtx_, jsArray, i++, jsValue ) )
    {
        throw smp::JsException();
    }

    return JS::ObjectValue( *jsArray );
}

} // namespace mozjs
