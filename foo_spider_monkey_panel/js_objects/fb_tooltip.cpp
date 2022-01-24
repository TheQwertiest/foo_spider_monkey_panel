#include <stdafx.h>

#include "fb_tooltip.h"

#include <js_engine/js_to_native_invoker.h>

#include <js_utils/js_error_helper.h>
#include <js_utils/js_hwnd_helpers.h>
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
    JsFbTooltip::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "FbTooltip",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( Activate, JsFbTooltip::Activate )
MJS_DEFINE_JS_FN_FROM_NATIVE( Deactivate, JsFbTooltip::Deactivate )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetDelayTime, JsFbTooltip::GetDelayTime )
MJS_DEFINE_JS_FN_FROM_NATIVE( SetDelayTime, JsFbTooltip::SetDelayTime )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( SetFont, JsFbTooltip::SetFont, JsFbTooltip::SetFontWithOpt, 2 )
MJS_DEFINE_JS_FN_FROM_NATIVE( SetMaxWidth, JsFbTooltip::SetMaxWidth )
MJS_DEFINE_JS_FN_FROM_NATIVE( TrackPosition, JsFbTooltip::TrackPosition )

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "Activate", Activate, 0, kDefaultPropsFlags ),
        JS_FN( "Deactivate", Deactivate, 0, kDefaultPropsFlags ),
        JS_FN( "GetDelayTime", GetDelayTime, 1, kDefaultPropsFlags ),
        JS_FN( "SetDelayTime", SetDelayTime, 2, kDefaultPropsFlags ),
        JS_FN( "SetFont", SetFont, 1, kDefaultPropsFlags ),
        JS_FN( "SetMaxWidth", SetMaxWidth, 1, kDefaultPropsFlags ),
        JS_FN( "TrackPosition", TrackPosition, 2, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_Text, JsFbTooltip::get_Text )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_Text, JsFbTooltip::put_Text )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_TrackActivate, JsFbTooltip::put_TrackActivate )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSGS( "Text", get_Text, put_Text, kDefaultPropsFlags ),
        JS_PSGS( "TrackActivate", DummyGetter, put_TrackActivate, kDefaultPropsFlags ),
        JS_PS_END,
    } );

} // namespace

namespace mozjs
{

const JSClass JsFbTooltip::JsClass = jsClass;
const JSFunctionSpec* JsFbTooltip::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsFbTooltip::JsProperties = jsProperties.data();
const JsPrototypeId JsFbTooltip::PrototypeId = JsPrototypeId::FbTooltip;

JsFbTooltip::JsFbTooltip( JSContext* cx, HWND hParentWnd )
    : pJsCtx_( cx )
    , hParentWnd_( hParentWnd )
    , tipBuffer_( TEXT( SMP_NAME ) )
{
    hTooltipWnd_ = CreateWindowEx(
        WS_EX_TOPMOST,
        TOOLTIPS_CLASS,
        nullptr,
        WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        hParentWnd_,
        nullptr,
        core_api::get_my_instance(),
        nullptr );
    qwr::error::CheckWinApi( hTooltipWnd_, "CreateWindowEx" );

    qwr::final_action autoHwnd( [hTooltipWnd = hTooltipWnd_] {
        if ( IsWindow( hTooltipWnd ) )
        {
            DestroyWindow( hTooltipWnd );
        }
    } );

    // Original position
    SetWindowPos( hTooltipWnd_, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );

    toolInfo_ = std::make_unique<TOOLINFO>();
    // Set up tooltip information.
    memset( toolInfo_.get(), 0, sizeof( TOOLINFO ) );

    toolInfo_->cbSize = sizeof( TOOLINFO );
    toolInfo_->uFlags = TTF_IDISHWND | TTF_SUBCLASS | TTF_TRANSPARENT;
    toolInfo_->hinst = core_api::get_my_instance();
    toolInfo_->hwnd = hParentWnd;
    toolInfo_->uId = (UINT_PTR)hParentWnd;
    toolInfo_->lpszText = const_cast<wchar_t*>( tipBuffer_.c_str() ); // we need to have text here, otherwise tooltip will glitch out

    bool bRet = SendMessage( hTooltipWnd_, TTM_ADDTOOL, 0, (LPARAM)toolInfo_.get() );
    qwr::error::CheckWinApi( bRet, "SendMessage(TTM_ADDTOOL)" );
    SendMessage( hTooltipWnd_, TTM_ACTIVATE, FALSE, 0 );

    autoHwnd.cancel();
}

std::unique_ptr<JsFbTooltip>
JsFbTooltip::CreateNative( JSContext* cx, HWND hParentWnd )
{
    qwr::QwrException::ExpectTrue( hParentWnd, "Internal error: hParentWnd is null" );

    return std::unique_ptr<JsFbTooltip>( new JsFbTooltip( cx, hParentWnd ) );
}

size_t JsFbTooltip::GetInternalSize( HWND /*hParentWnd*/ )
{
    return sizeof( LOGFONT ) + sizeof( TOOLINFO );
}

void JsFbTooltip::PrepareForGc()
{
    if ( hTooltipWnd_ && IsWindow( hTooltipWnd_ ) )
    {
        Deactivate();
        DestroyWindow( hTooltipWnd_ );
        hTooltipWnd_ = nullptr;
    }
}

void JsFbTooltip::Activate()
{
    SendMessage( hTooltipWnd_, TTM_ACTIVATE, TRUE, 0 );
}

void JsFbTooltip::Deactivate()
{
    SendMessage( hTooltipWnd_, TTM_ACTIVATE, FALSE, 0 );
}

uint32_t JsFbTooltip::GetDelayTime( uint32_t type )
{
    qwr::QwrException::ExpectTrue( type >= TTDT_AUTOMATIC && type <= TTDT_INITIAL, "Invalid delay type: {}", type );

    return SendMessage( hTooltipWnd_, TTM_GETDELAYTIME, type, 0 );
}

void JsFbTooltip::SetDelayTime( uint32_t type, int32_t time )
{
    qwr::QwrException::ExpectTrue( type >= TTDT_AUTOMATIC && type <= TTDT_INITIAL, "Invalid delay type: {}", type );

    SendMessage( hTooltipWnd_, TTM_SETDELAYTIME, type, static_cast<LPARAM>( static_cast<int>( MAKELONG( time, 0 ) ) ) );
}

void JsFbTooltip::SetFont( const std::wstring& fontName, int32_t fontSize, uint32_t fontStyle )
{
    LOGFONTW logfont;
    logfont::Make( fontName, fontSize, fontStyle, logfont );

#if FONT_CACHE_ABSOLUTE_HEIGHT
    const HWND wnd = GetPanelHwndForCurrentGlobal( pJsCtx_ );
    const HDC dc = GetDC( wnd );
    qwr::final_action autoHdcReleaser( [wnd, dc] { ReleaseDC( wnd, dc ); } );
    logfont::Normalize( dc, logfont );
#endif

    fontcache::shared_hfont hfont = fontcache::Cache( logfont );

    if ( font == hfont )
        return;

    font = hfont;

    SendMessage( hTooltipWnd_, WM_SETFONT, (WPARAM)font.get(), MAKELPARAM( FALSE, 0 ) );
}

void JsFbTooltip::SetFontWithOpt( size_t optArgCount,
                                  const std::wstring& fontName, int32_t fontSize, uint32_t fontStyle )
{
    switch ( optArgCount )
    {
    case 0:
        return SetFont( fontName, fontSize, fontStyle );
    case 1:
        return SetFont( fontName, fontSize );
    case 2:
        return SetFont( fontName );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsFbTooltip::SetMaxWidth( uint32_t width )
{
    SendMessage( hTooltipWnd_, TTM_SETMAXTIPWIDTH, 0, width );
}

void JsFbTooltip::TrackPosition( int x, int y )
{
    POINT pt{ x, y };
    ClientToScreen( hParentWnd_, &pt );
    SendMessage( hTooltipWnd_, TTM_TRACKPOSITION, 0, MAKELONG( pt.x, pt.y ) );
}

std::wstring JsFbTooltip::get_Text()
{
    return tipBuffer_;
}

void JsFbTooltip::put_Text( const std::wstring& text )
{
    tipBuffer_ = text;
    toolInfo_->lpszText = tipBuffer_.data();
    SendMessage( hTooltipWnd_, TTM_UPDATETIPTEXT, 0, (LPARAM)toolInfo_.get() );
}

void JsFbTooltip::put_TrackActivate( bool activate )
{
    if ( activate )
    {
        toolInfo_->uFlags |= TTF_TRACK | TTF_ABSOLUTE;
    }
    else
    {
        toolInfo_->uFlags &= ~( TTF_TRACK | TTF_ABSOLUTE );
    }

    SendMessage( hTooltipWnd_, TTM_TRACKACTIVATE, static_cast<WPARAM>( activate ), (LPARAM)toolInfo_.get() );
}

} // namespace mozjs
