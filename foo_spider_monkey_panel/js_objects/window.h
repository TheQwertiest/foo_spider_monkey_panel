#pragma once

#include <js_objects/object_base.h>

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;

namespace smp::panel
{
class js_panel_window;
}

namespace smp::com
{
class IDropTargetImpl;
}

namespace mozjs
{

class FbProperties;

class JsWindow
    : public JsObjectBase<JsWindow>
{
public:
    static constexpr bool HasProto = false;
    static constexpr bool HasProxy = false;
    static constexpr bool HasPostCreate = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;

public:
    ~JsWindow();

    static std::unique_ptr<JsWindow> CreateNative( JSContext* cx, smp::panel::js_panel_window& parentPanel );
    static size_t GetInternalSize( const smp::panel::js_panel_window& parentPanel );

public:
    void PrepareForGc();

public: // methods
    void ClearInterval( uint32_t intervalId );
    void ClearTimeout( uint32_t timeoutId );
    JSObject* CreatePopupMenu();
    JSObject* CreateThemeManager( const std::wstring& classid );
    JSObject* CreateTooltip( const std::wstring& name = L"Segoe UI", float pxSize = 12, uint32_t style = 0 );
    JSObject* CreateTooltipWithOpt( size_t optArgCount, const std::wstring& name, float pxSize, uint32_t style );
    void DefinePanel( const pfc::string8_fast& name, JS::HandleValue options = JS::UndefinedHandleValue );
    void DefinePanelWithOpt( size_t optArgCount, const pfc::string8_fast& name, JS::HandleValue options = JS::UndefinedHandleValue );
    uint32_t GetColourCUI( uint32_t type, const std::wstring& guidstr = L"" );
    uint32_t GetColourCUIWithOpt( size_t optArgCount, uint32_t type, const std::wstring& guidstr );
    uint32_t GetColourDUI( uint32_t type );
    JSObject* GetFontCUI( uint32_t type, const std::wstring& guidstr = L"" );
    JSObject* GetFontCUIWithOpt( size_t optArgCount, uint32_t type, const std::wstring& guidstr );
    JSObject* GetFontDUI( uint32_t type );
    JS::Value GetProperty( const std::wstring& name, JS::HandleValue defaultval = JS::NullHandleValue );
    JS::Value GetPropertyWithOpt( size_t optArgCount, const std::wstring& name, JS::HandleValue defaultval );
    void NotifyOthers( const std::wstring& name, JS::HandleValue info );
    void Reload();
    void Repaint( bool force = false );
    void RepaintWithOpt( size_t optArgCount, bool force );
    void RepaintRect( uint32_t x, uint32_t y, uint32_t w, uint32_t h, bool force = false );
    void RepaintRectWithOpt( size_t optArgCount, uint32_t x, uint32_t y, uint32_t w, uint32_t h, bool force );
    void SetCursor( uint32_t id );
    uint32_t SetInterval( JS::HandleValue func, uint32_t delay );
    void SetProperty( const std::wstring& name, JS::HandleValue val = JS::NullHandleValue );
    void SetPropertyWithOpt( size_t optArgCount, const std::wstring& name, JS::HandleValue val );
    uint32_t SetTimeout( JS::HandleValue func, uint32_t delay );
    void ShowConfigure();
    void ShowProperties();

public: // props
    uint32_t get_DlgCode();
    uint32_t get_Height();
    uint32_t get_ID();
    uint32_t get_InstanceType();
    bool get_IsTransparent();
    bool get_IsVisible();
    uint32_t get_MaxHeight();
    uint32_t get_MaxWidth();
    uint32_t get_MemoryLimit();
    uint32_t get_MinHeight();
    uint32_t get_MinWidth();
    pfc::string8_fast get_Name();
    uint64_t get_PanelMemoryUsage();
    uint64_t get_TotalMemoryUsage();
    uint32_t get_Width();
    void put_DlgCode( uint32_t code );
    void put_MaxHeight( uint32_t height );
    void put_MaxWidth( uint32_t width );
    void put_MinHeight( uint32_t height );
    void put_MinWidth( uint32_t width );

private:
    JsWindow( JSContext* cx, smp::panel::js_panel_window& parentPanel, std::unique_ptr<FbProperties> fbProperties );

private:
    JSContext* pJsCtx_;
    smp::panel::js_panel_window& parentPanel_;

    bool isFinalized_ = false; ///< if true, then parentPanel_ might be already inaccessible

    bool isPanelDefined_ = false;
    std::unique_ptr<FbProperties> fbProperties_;
    CComPtr<smp::com::IDropTargetImpl> dropTargetHandler_;
};

} // namespace mozjs
