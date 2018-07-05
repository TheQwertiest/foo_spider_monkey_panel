#include <stdafx.h>
#include "fb_tooltip.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>

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
    JsFinalizeOp<JsFbTooltip>,
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


MJS_DEFINE_JS_TO_NATIVE_FN( JsFbTooltip, Activate )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbTooltip, Deactivate )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbTooltip, GetDelayTime )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbTooltip, SetDelayTime )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbTooltip, SetMaxWidth )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbTooltip, TrackPosition )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "Activate", Activate, 0, DefaultPropsFlags() ),
    JS_FN( "Deactivate", Deactivate, 0, DefaultPropsFlags() ),
    JS_FN( "GetDelayTime", GetDelayTime, 1, DefaultPropsFlags() ),
    JS_FN( "SetDelayTime", SetDelayTime, 2, DefaultPropsFlags() ),
    JS_FN( "SetMaxWidth", SetMaxWidth, 1, DefaultPropsFlags() ),
    JS_FN( "TrackPosition", TrackPosition, 2, DefaultPropsFlags() ),
    JS_FS_END
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsFbTooltip, get_Text )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbTooltip, put_Text )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbTooltip, put_TrackActivate )

const JSPropertySpec jsProperties[] = {
    JS_PSGS( "Text", get_Text, put_Text, DefaultPropsFlags() ),
    JS_PSGS( "TrackActivate", DummyGetter, put_TrackActivate, DefaultPropsFlags() ),
    JS_PS_END
};

}

namespace mozjs
{


JsFbTooltip::JsFbTooltip( JSContext* cx, HWND hParentWnd, PanelTooltipParam& p_param_ptr )
    : pJsCtx_( cx )
    , hParentWnd_( hParentWnd )
    , panelTooltipParam_( p_param_ptr )
    , tipBuffer_( PFC_WIDESTRING( JSP_NAME ) )
{
    // TODO: move to Create

    hTooltipWnd_ = CreateWindowEx(
        WS_EX_TOPMOST,
        TOOLTIPS_CLASS,
        NULL,
        WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        hParentWnd_,
        NULL,
        core_api::get_my_instance(),
        NULL );
    // IF_WINAPI_FAILED_RETURN_WITH_REPORT( pJsCtx_, !!hTooltipWnd_, std::nullopt, CreateWindowEx );

    // Original position
    SetWindowPos( hTooltipWnd_, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );

    // Set up tooltip information.
    memset( &toolInfo_, 0, sizeof( toolInfo_ ) );

    toolInfo_.cbSize = sizeof( toolInfo_ );
    toolInfo_.uFlags = TTF_IDISHWND | TTF_SUBCLASS | TTF_TRANSPARENT;
    toolInfo_.hinst = core_api::get_my_instance();
    toolInfo_.hwnd = hParentWnd_;
    toolInfo_.uId = (UINT_PTR)hParentWnd_;
    toolInfo_.lpszText = (LPWSTR)tipBuffer_.c_str();

    HFONT hFont = CreateFont(
        -(INT)panelTooltipParam_.fontSize,
        0,
        0,
        0,
        (panelTooltipParam_.fontStyle & Gdiplus::FontStyleBold) ? FW_BOLD : FW_NORMAL,
        (panelTooltipParam_.fontStyle & Gdiplus::FontStyleItalic) ? TRUE : FALSE,
        (panelTooltipParam_.fontStyle & Gdiplus::FontStyleUnderline) ? TRUE : FALSE,
        (panelTooltipParam_.fontStyle & Gdiplus::FontStyleStrikeout) ? TRUE : FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        panelTooltipParam_.fontName.c_str() );
    // IF_WINAPI_FAILED_RETURN_WITH_REPORT( pJsCtx_, !!hFont, std::nullopt, CreateFont );

    SendMessage( hTooltipWnd_, TTM_ADDTOOL, 0, (LPARAM)&toolInfo_ );
    SendMessage( hTooltipWnd_, TTM_ACTIVATE, FALSE, 0 );
    SendMessage( hTooltipWnd_, WM_SETFONT, (WPARAM)hFont, MAKELPARAM( FALSE, 0 ) );

    panelTooltipParam_.hTooltip = hTooltipWnd_;
    panelTooltipParam_.tooltipSize.cx = -1;
    panelTooltipParam_.tooltipSize.cy = -1;
}


JsFbTooltip::~JsFbTooltip()
{
    if ( hTooltipWnd_ && IsWindow( hTooltipWnd_ ) )
    {
        DestroyWindow( hTooltipWnd_ );
    }
}

JSObject* JsFbTooltip::Create( JSContext* cx, HWND hParentWnd, PanelTooltipParam& p_param_ptr )
{
    JS::RootedObject jsObj( cx,
                            JS_NewObject( cx, &jsClass ) );
    if ( !jsObj )
    {
        return nullptr;
    }

    if ( !JS_DefineFunctions( cx, jsObj, jsFunctions )
         || !JS_DefineProperties( cx, jsObj, jsProperties ) )
    {
        return nullptr;
    }

    JS_SetPrivate( jsObj, new JsFbTooltip( cx, hParentWnd, p_param_ptr ) );

    return jsObj;
}

const JSClass& JsFbTooltip::GetClass()
{
    return jsClass;
}

std::optional<std::nullptr_t>
JsFbTooltip::Activate()
{
    SendMessage( hTooltipWnd_, TTM_ACTIVATE, TRUE, 0 );
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbTooltip::Deactivate()
{
    SendMessage( hTooltipWnd_, TTM_ACTIVATE, FALSE, 0 );
    return nullptr;
}

std::optional<uint32_t>
JsFbTooltip::GetDelayTime( uint32_t type )
{
    if ( type < TTDT_AUTOMATIC || type > TTDT_INITIAL )
    {
        JS_ReportErrorASCII( pJsCtx_, "Invalid delay type" );
        return std::nullopt;
    }

    return SendMessage( hTooltipWnd_, TTM_GETDELAYTIME, type, 0 );    
}

std::optional<std::nullptr_t>
JsFbTooltip::SetDelayTime( uint32_t type, int32_t time )
{   
    if ( type < TTDT_AUTOMATIC || type > TTDT_INITIAL )
    {
        JS_ReportErrorASCII( pJsCtx_, "Invalid delay type" );
        return std::nullopt;
    }

    SendMessage( hTooltipWnd_, TTM_SETDELAYTIME, type, (LPARAM)(INT)MAKELONG( time, 0 ) );
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbTooltip::SetMaxWidth( uint32_t width )
{
    SendMessage( hTooltipWnd_, TTM_SETMAXTIPWIDTH, 0, width );
    return nullptr;
}

std::optional<std::nullptr_t> 
JsFbTooltip::TrackPosition( int x, int y )
{
    POINT pt = { x, y };
    ClientToScreen( hParentWnd_, &pt );
    SendMessage( hTooltipWnd_, TTM_TRACKPOSITION, 0, MAKELONG( pt.x, pt.y ) );
    return nullptr;
}

std::optional<std::wstring>
JsFbTooltip::get_Text()
{    
    return tipBuffer_;
}

std::optional<std::nullptr_t> 
JsFbTooltip::put_Text( const std::wstring& text )
{
    tipBuffer_.assign(text);
    toolInfo_.lpszText = (LPWSTR)tipBuffer_.c_str();
    SendMessage( hTooltipWnd_, TTM_SETTOOLINFO, 0, (LPARAM)&toolInfo_ );
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbTooltip::put_TrackActivate( bool activate )
{
    if ( activate )
    {
        toolInfo_.uFlags |= TTF_TRACK | TTF_ABSOLUTE;
    }
    else
    {
        toolInfo_.uFlags &= ~(TTF_TRACK | TTF_ABSOLUTE);
    }

    SendMessage( hTooltipWnd_, TTM_TRACKACTIVATE, activate, (LPARAM)&toolInfo_ );
    return nullptr;
}

}
