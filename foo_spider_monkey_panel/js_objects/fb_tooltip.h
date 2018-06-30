#pragma once

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
    struct panel_tooltip_param
    {
        HWND hTooltip;
        SIZE tooltipSize_;

        std::wstring fontName;
        float fontSize;
        uint32_t fontStyle;

        panel_tooltip_param() : hTooltip( nullptr )
        {
        }
    };

    using panel_tooltip_param_ptr = std::shared_ptr<panel_tooltip_param>;

public:
    ~JsFbTooltip();

    static JSObject* Create( JSContext* cx, HWND p_wndparent, const panel_tooltip_param_ptr& p_param_ptr );

    static const JSClass& GetClass();

public:
    std::optional<std::nullptr_t> Activate();
    std::optional<std::nullptr_t> Deactivate();
    // TODO: param need casting!
    // wParam = (WPARAM)(DWORD)dwDuration;
    // lParam = (LPARAM)(INT)MAKELONG( iTime, 0 );
    std::optional<int32_t> GetDelayTime( uint32_t type );
    // TODO: param need casting!
    std::optional<std::nullptr_t> SetDelayTime( uint32_t type, uint32_t time );
    std::optional<std::nullptr_t> SetMaxWidth( uint32_t width );
    std::optional<std::nullptr_t> TrackPosition( int x, int y );

public:
    std::optional<std::wstring> get_Text();
    std::optional<std::nullptr_t> put_Text( std::wstring text );
    std::optional<std::nullptr_t> put_TrackActivate( bool activate );

private:
    JsFbTooltip( JSContext* cx, HWND p_wndparent, const panel_tooltip_param_ptr& p_param_ptr );
    JsFbTooltip( const JsFbTooltip& ) = delete;
    JsFbTooltip& operator=( const JsFbTooltip& ) = delete;

private:
    JSContext * pJsCtx_ = nullptr;

    HWND m_wndtooltip;
    HWND m_wndparent;
    BSTR m_tip_buffer;
    TOOLINFO m_ti;
    panel_tooltip_param_ptr m_panel_tooltip_param_ptr;
};

}
