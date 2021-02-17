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

namespace mozjs
{

class FbProperties;
class JsFbTooltip;

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
    // @remark No need to cleanup JS here, since it must be performed manually beforehand anyway
    ~JsWindow() override;

    static std::unique_ptr<JsWindow> CreateNative( JSContext* cx, smp::panel::js_panel_window& parentPanel );
    static size_t GetInternalSize( const smp::panel::js_panel_window& parentPanel );

public:
    static void Trace( JSTracer* trc, JSObject* obj );
    void PrepareForGc();
    [[nodiscard]] HWND GetHwnd() const;

public: // methods
    void ClearInterval( uint32_t intervalId ) const;
    void ClearTimeout( uint32_t timeoutId ) const;
    JSObject* CreatePopupMenu();
    JSObject* CreateThemeManager( const std::wstring& classid );
    // TODO v2: remove
    JSObject* CreateTooltip( const std::wstring& name = L"Segoe UI", uint32_t pxSize = 12, uint32_t style = 0 );
    // TODO v2: remove
    JSObject* CreateTooltipWithOpt( size_t optArgCount, const std::wstring& name, uint32_t pxSize, uint32_t style );
    // TODO v2: remove
    void DefinePanel( const qwr::u8string& name, JS::HandleValue options = JS::UndefinedHandleValue );
    // TODO v2: remove
    void DefinePanelWithOpt( size_t optArgCount, const qwr::u8string& name, JS::HandleValue options = JS::UndefinedHandleValue );
    void DefineScript( const qwr::u8string& name, JS::HandleValue options = JS::UndefinedHandleValue );
    void DefineScriptWithOpt( size_t optArgCount, const qwr::u8string& name, JS::HandleValue options = JS::UndefinedHandleValue );
    void EditScript();
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
    uint32_t SetInterval( JS::HandleValue func, uint32_t delay, JS::HandleValueArray funcArgs = JS::HandleValueArray{ JS::UndefinedHandleValue } );
    uint32_t SetIntervalWithOpt( size_t optArgCount, JS::HandleValue func, uint32_t delay, JS::HandleValueArray funcArgs );
    void SetProperty( const std::wstring& name, JS::HandleValue val = JS::NullHandleValue );
    void SetPropertyWithOpt( size_t optArgCount, const std::wstring& name, JS::HandleValue val );
    uint32_t SetTimeout( JS::HandleValue func, uint32_t delay, JS::HandleValueArray funcArgs = JS::HandleValueArray{ JS::UndefinedHandleValue } );
    uint32_t SetTimeoutWithOpt( size_t optArgCount, JS::HandleValue func, uint32_t delay, JS::HandleValueArray funcArgs );
    void ShowConfigure();
    void ShowConfigureV2();
    void ShowProperties();

public: // props
    uint32_t get_DlgCode();
    uint32_t get_Height();
    uint32_t get_ID() const;
    uint32_t get_InstanceType();
    bool get_IsTransparent();
    bool get_IsVisible();
    JSObject* get_JsMemoryStats();
    uint32_t get_MaxHeight();
    uint32_t get_MaxWidth();
    // TODO v2: remove
    uint32_t get_MemoryLimit() const;
    uint32_t get_MinHeight();
    uint32_t get_MinWidth();
    qwr::u8string get_Name();
    // TODO v2: remove
    uint64_t get_PanelMemoryUsage();
    JSObject* get_ScriptInfo();
    JSObject* get_Tooltip();
    // TODO v2: remove
    uint64_t get_TotalMemoryUsage() const;
    uint32_t get_Width();
    void put_DlgCode( uint32_t code );
    void put_MaxHeight( uint32_t height );
    void put_MaxWidth( uint32_t width );
    void put_MinHeight( uint32_t height );
    void put_MinWidth( uint32_t width );

private:
    JsWindow( JSContext* cx, smp::panel::js_panel_window& parentPanel, std::unique_ptr<FbProperties> fbProperties );

    struct DefineScriptOptions
    {
        qwr::u8string author;
        qwr::u8string version;
        struct Features
        {
            bool dragAndDrop = false;
            bool grabFocus = true;
        } features;
    };
    DefineScriptOptions ParseDefineScriptOptions( JS::HandleValue options );

private:
    JSContext* pJsCtx_;
    smp::panel::js_panel_window& parentPanel_;

    bool isFinalized_ = false; ///< if true, then parentPanel_ might be already inaccessible

    bool isScriptDefined_ = false;
    std::unique_ptr<FbProperties> fbProperties_;

    JS::PersistentRootedObject jsTooltip_;
    JsFbTooltip* pNativeTooltip_ = nullptr;
};

} // namespace mozjs
