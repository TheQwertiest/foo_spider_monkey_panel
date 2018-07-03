#pragma once

#include <panel_tooltip_param.h>

#include <optional>
#include <memory>
#include <string>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsFbTooltip
{
public:
    ~JsFbTooltip();

    static JSObject* Create( JSContext* cx, HWND hParentWnd, smp::PanelTooltipParam& p_param_ptr );

    static const JSClass& GetClass();

public:
    std::optional<std::nullptr_t> Activate();
    std::optional<std::nullptr_t> Deactivate();
    std::optional<uint32_t> GetDelayTime( uint32_t type );
    std::optional<std::nullptr_t> SetDelayTime( uint32_t type, int32_t time );
    std::optional<std::nullptr_t> SetMaxWidth( uint32_t width );
    std::optional<std::nullptr_t> TrackPosition( int x, int y );

public:
    std::optional<std::wstring> get_Text();
    std::optional<std::nullptr_t> put_Text( const std::wstring& text );
    std::optional<std::nullptr_t> put_TrackActivate( bool activate );

private:
    JsFbTooltip( JSContext* cx, HWND hParentWnd, smp::PanelTooltipParam& p_param_ptr );
    JsFbTooltip( const JsFbTooltip& ) = delete;
    JsFbTooltip& operator=( const JsFbTooltip& ) = delete;

private:
    JSContext * pJsCtx_ = nullptr;

    HWND hTooltipWnd_;
    HWND hParentWnd_;
    std::wstring tipBuffer_;
    TOOLINFO toolInfo_;
    smp::PanelTooltipParam& panelTooltipParam_;
};

}
