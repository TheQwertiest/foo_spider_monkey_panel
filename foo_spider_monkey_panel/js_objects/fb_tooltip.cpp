#include <stdafx.h>

#include "fb_tooltip.h"

#include <js_engine/js_to_native_invoker.h>
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
    , pFont_( smp::gdi::CreateUniquePtr<HFONT>( nullptr ) )
{
    qwr::final_action autoHwnd( [&] {
        if ( tooltipManual_.IsWindow() )
        {
            if ( pToolinfoManual_ )
            {
                tooltipManual_.DelTool( pToolinfoManual_.get() );
            }
            tooltipManual_.DestroyWindow();
        }
    } );

    tooltipManual_.Create( hParentWnd_ );
    qwr::error::CheckWinApi( tooltipManual_, "tooltip::Create" );

    // Original position
    tooltipManual_.SetWindowPos( HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );

    // Set up tooltip information.
    // note: we need to have text here, otherwise tooltip will glitch out
    pToolinfoManual_ = std::make_unique<CToolInfo>( TTF_IDISHWND | TTF_SUBCLASS, hParentWnd_, (UINT_PTR)hParentWnd_, nullptr, const_cast<wchar_t*>( tipBuffer_.c_str() ) );

    auto bRet = tooltipManual_.AddTool( pToolinfoManual_.get() );
    qwr::error::CheckWinApi( bRet, "tooltip::AddTool" );

    tooltipManual_.Activate( FALSE );

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
    if ( tooltipManual_.IsWindow() )
    {
        Deactivate();
        if ( pToolinfoManual_ )
        {
            tooltipManual_.DelTool( pToolinfoManual_.get() );
        }
        tooltipManual_.DestroyWindow();
    }
}

void JsFbTooltip::Activate()
{
    tooltipManual_.Activate( TRUE );
}

void JsFbTooltip::Deactivate()
{
    tooltipManual_.Activate( FALSE );
}

uint32_t JsFbTooltip::GetDelayTime( uint32_t type )
{
    qwr::QwrException::ExpectTrue( type >= TTDT_AUTOMATIC && type <= TTDT_INITIAL, "Invalid delay type: {}", type );

    return tooltipManual_.GetDelayTime( type );
}

void JsFbTooltip::SetDelayTime( uint32_t type, int32_t time )
{
    qwr::QwrException::ExpectTrue( type >= TTDT_AUTOMATIC && type <= TTDT_INITIAL, "Invalid delay type: {}", type );

    tooltipManual_.SetDelayTime( type, time );
}

void JsFbTooltip::SetFont( const std::wstring& name, uint32_t pxSize, uint32_t style )
{
    fontName_ = name;
    fontSize_ = pxSize;
    fontStyle_ = style;

    if ( !fontName_.empty() )
    {
        pFont_.reset( CreateFont(
            // from msdn: "< 0, The font mapper transforms this value into device units
            //             and matches its absolute value against the character height of the available fonts."
            -static_cast<int>( fontSize_ ),
            0,
            0,
            0,
            ( fontStyle_ & Gdiplus::FontStyleBold ) ? FW_BOLD : FW_NORMAL,
            ( fontStyle_ & Gdiplus::FontStyleItalic ) ? TRUE : FALSE,
            ( fontStyle_ & Gdiplus::FontStyleUnderline ) ? TRUE : FALSE,
            ( fontStyle_ & Gdiplus::FontStyleStrikeout ) ? TRUE : FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            fontName_.c_str() ) );
        qwr::error::CheckWinApi( !!pFont_, "CreateFont" );
        tooltipManual_.SetFont( pFont_.get(), FALSE );
    }
}

void JsFbTooltip::SetFontWithOpt( size_t optArgCount, const std::wstring& name, uint32_t pxSize, uint32_t style )
{
    switch ( optArgCount )
    {
    case 0:
        return SetFont( name, pxSize, style );
    case 1:
        return SetFont( name, pxSize );
    case 2:
        return SetFont( name );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsFbTooltip::SetMaxWidth( uint32_t width )
{
    tooltipManual_.SetMaxTipWidth( width );
}

void JsFbTooltip::TrackPosition( int x, int y )
{
    POINT pt{ x, y };
    ClientToScreen( hParentWnd_, &pt );
    tooltipManual_.TrackPosition( pt.x, pt.y );
}

std::wstring JsFbTooltip::get_Text()
{
    return tipBuffer_;
}

void JsFbTooltip::put_Text( const std::wstring& text )
{
    tipBuffer_ = text;
    pToolinfoManual_->lpszText = tipBuffer_.data();
    tooltipManual_.SetToolInfo( pToolinfoManual_.get() );
}

void JsFbTooltip::put_TrackActivate( bool activate )
{
    if ( activate )
    {
        pToolinfoManual_->uFlags |= TTF_TRACK | TTF_ABSOLUTE;
    }
    else
    {
        pToolinfoManual_->uFlags &= ~( TTF_TRACK | TTF_ABSOLUTE );
    }

    tooltipManual_.TrackActivate( pToolinfoManual_.get(), activate );
}

} // namespace mozjs
