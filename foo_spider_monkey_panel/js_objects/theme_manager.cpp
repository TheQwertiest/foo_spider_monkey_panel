#include <stdafx.h>

#include "theme_manager.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/gdi_graphics.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>

#include <qwr/final_action.h>
#include <qwr/winapi_error_helpers.h>

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
MJS_DEFINE_JS_FN_FROM_NATIVE( IsThemePartDefined, JsThemeManager::IsThemePartDefined )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( SetPartAndStateID, JsThemeManager::SetPartAndStateID, JsThemeManager::SetPartAndStateIDWithOpt, 1 )

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "DrawThemeBackground", DrawThemeBackground, 5, kDefaultPropsFlags ),
        JS_FN( "IsThemePartDefined", IsThemePartDefined, 1, kDefaultPropsFlags ),
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

    HDC dc = graphics->GetHDC();
    qwr::final_action autoHdcReleaser( [graphics, dc]() { graphics->ReleaseHDC( dc ); } );

    const RECT rc{ x, y, static_cast<LONG>( x + w ), static_cast<LONG>( y + h ) };
    const RECT clip_rc{ clip_x, clip_y, static_cast<LONG>( clip_x + clip_y ), static_cast<LONG>( clip_w + clip_h ) };
    LPCRECT pclip_rc = ( !clip_x && !clip_y && !clip_w && !clip_h ) ? nullptr : &clip_rc;

    HRESULT hr = ::DrawThemeBackground( hTheme_, dc, partId_, stateId_, &rc, pclip_rc );
    qwr::error::CheckHR( hr, u8"DrawThemeBackground" );
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

bool JsThemeManager::IsThemePartDefined( int32_t partid, int32_t stateId )
{
    return ::IsThemePartDefined( hTheme_, partid, stateId );
}

void JsThemeManager::SetPartAndStateID( int32_t partid, int32_t stateId )
{
    partId_ = partid;
    stateId_ = stateId;
}

void JsThemeManager::SetPartAndStateIDWithOpt( size_t optArgCount, int32_t partid, int32_t stateId )
{
    switch ( optArgCount )
    {
    case 0:
        return SetPartAndStateID( partid, stateId );
    case 1:
        return SetPartAndStateID( partid );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

} // namespace mozjs
