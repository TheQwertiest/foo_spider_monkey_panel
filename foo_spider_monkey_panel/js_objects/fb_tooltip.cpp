#include <stdafx.h>

#include "fb_tooltip.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <utils/array_x.h>
#include <utils/scope_helpers.h>
#include <utils/winapi_error_helpers.h>

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
    DefaultClassFlags(),
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( Activate, JsFbTooltip::Activate )
MJS_DEFINE_JS_FN_FROM_NATIVE( Deactivate, JsFbTooltip::Deactivate )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetDelayTime, JsFbTooltip::GetDelayTime )
MJS_DEFINE_JS_FN_FROM_NATIVE( SetDelayTime, JsFbTooltip::SetDelayTime )
MJS_DEFINE_JS_FN_FROM_NATIVE( SetMaxWidth, JsFbTooltip::SetMaxWidth )
MJS_DEFINE_JS_FN_FROM_NATIVE( TrackPosition, JsFbTooltip::TrackPosition )

constexpr auto jsFunctions = smp::to_array<JSFunctionSpec>(
    {
        JS_FN( "Activate", Activate, 0, DefaultPropsFlags() ),
        JS_FN( "Deactivate", Deactivate, 0, DefaultPropsFlags() ),
        JS_FN( "GetDelayTime", GetDelayTime, 1, DefaultPropsFlags() ),
        JS_FN( "SetDelayTime", SetDelayTime, 2, DefaultPropsFlags() ),
        JS_FN( "SetMaxWidth", SetMaxWidth, 1, DefaultPropsFlags() ),
        JS_FN( "TrackPosition", TrackPosition, 2, DefaultPropsFlags() ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_Text, JsFbTooltip::get_Text )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_Text, JsFbTooltip::put_Text )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_TrackActivate, JsFbTooltip::put_TrackActivate )

constexpr auto jsProperties = smp::to_array<JSPropertySpec>(
    {
        JS_PSGS( "Text", get_Text, put_Text, DefaultPropsFlags() ),
        JS_PSGS( "TrackActivate", DummyGetter, put_TrackActivate, DefaultPropsFlags() ),
        JS_PS_END,
    } );

} // namespace

namespace mozjs
{

const JSClass JsFbTooltip::JsClass = jsClass;
const JSFunctionSpec* JsFbTooltip::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsFbTooltip::JsProperties = jsProperties.data();
const JsPrototypeId JsFbTooltip::PrototypeId = JsPrototypeId::FbTooltip;

JsFbTooltip::JsFbTooltip( JSContext* cx, HWND hParentWnd, smp::panel::PanelTooltipParam& p_param_ptr )
    : pJsCtx_( cx )
    , hParentWnd_( hParentWnd )
    , panelTooltipParam_( p_param_ptr )
    , tipBuffer_( TEXT( SMP_NAME ) )
    , pFont_( smp::gdi::CreateUniquePtr<HFONT>( nullptr ) )
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
    smp::error::CheckWinApi( hTooltipWnd_, "CreateWindowEx" );

    smp::utils::final_action autoHwnd( [hTooltipWnd = hTooltipWnd_] {
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

    pFont_.reset( CreateFont(
        // from msdn: "< 0, The font mapper transforms this value into device units
        //             and matches its absolute value against the character height of the available fonts."
        -static_cast<int>( panelTooltipParam_.fontSize ),
        0,
        0,
        0,
        ( panelTooltipParam_.fontStyle & Gdiplus::FontStyleBold ) ? FW_BOLD : FW_NORMAL,
        ( panelTooltipParam_.fontStyle & Gdiplus::FontStyleItalic ) ? TRUE : FALSE,
        ( panelTooltipParam_.fontStyle & Gdiplus::FontStyleUnderline ) ? TRUE : FALSE,
        ( panelTooltipParam_.fontStyle & Gdiplus::FontStyleStrikeout ) ? TRUE : FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        panelTooltipParam_.fontName.c_str() ) );
    smp::error::CheckWinApi( !!pFont_, "CreateFont" );

    bool bRet = SendMessage( hTooltipWnd_, TTM_ADDTOOL, 0, (LPARAM)toolInfo_.get() );
    smp::error::CheckWinApi( bRet, "SendMessage(TTM_ADDTOOL)" );
    SendMessage( hTooltipWnd_, TTM_ACTIVATE, FALSE, 0 );
    SendMessage( hTooltipWnd_, WM_SETFONT, (WPARAM)pFont_.get(), MAKELPARAM( FALSE, 0 ) );

    panelTooltipParam_.hTooltip = hTooltipWnd_;
    panelTooltipParam_.tooltipSize.cx = -1;
    panelTooltipParam_.tooltipSize.cy = -1;

    autoHwnd.cancel();
}

JsFbTooltip::~JsFbTooltip()
{
    if ( IsWindow( hTooltipWnd_ ) )
    {
        DestroyWindow( hTooltipWnd_ );
    }
}

std::unique_ptr<JsFbTooltip>
JsFbTooltip::CreateNative( JSContext* cx, HWND hParentWnd, panel::PanelTooltipParam& p_param_ptr )
{
    SmpException::ExpectTrue( hParentWnd, "Internal error: hParentWnd is null" );

    return std::unique_ptr<JsFbTooltip>( new JsFbTooltip( cx, hParentWnd, p_param_ptr ) );
}

size_t JsFbTooltip::GetInternalSize( HWND /*hParentWnd*/, const panel::PanelTooltipParam& /*p_param_ptr*/ )
{
    return sizeof( LOGFONT ) + sizeof( TOOLINFO );
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
    SmpException::ExpectTrue( type >= TTDT_AUTOMATIC && type <= TTDT_INITIAL, "Invalid delay type: {}", type );

    return SendMessage( hTooltipWnd_, TTM_GETDELAYTIME, type, 0 );
}

void JsFbTooltip::SetDelayTime( uint32_t type, int32_t time )
{
    SmpException::ExpectTrue( type >= TTDT_AUTOMATIC && type <= TTDT_INITIAL, "Invalid delay type: {}", type );

    SendMessage( hTooltipWnd_, TTM_SETDELAYTIME, type, static_cast<LPARAM>( static_cast<int>( MAKELONG( time, 0 ) ) ) );
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
    SendMessage( hTooltipWnd_, TTM_SETTOOLINFO, 0, (LPARAM)toolInfo_.get() );
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
