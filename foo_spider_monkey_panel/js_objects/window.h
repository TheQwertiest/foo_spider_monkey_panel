#pragma once

#include <js_objects/object_base.h>

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;

class js_panel_window;

namespace mozjs
{

class FbProperties;

class JsWindow
    : public JsObjectBase<JsWindow>
{
public:
    static constexpr bool HasProto = false;
    static constexpr bool HasProxy = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;

public:
    ~JsWindow();

    static std::unique_ptr<JsWindow> CreateNative( JSContext* cx, js_panel_window& parentPanel );

public:
    void RemoveHeapTracer();

public: // methods
    std::optional<std::nullptr_t> ClearInterval( uint32_t intervalId );
    std::optional<std::nullptr_t> ClearTimeout( uint32_t timeoutId );
    std::optional<JSObject*> CreatePopupMenu();
    std::optional<JSObject*> CreateThemeManager( const std::wstring& classid );
    std::optional<JSObject*> CreateTooltip( const std::wstring& name = L"Segoe UI", float pxSize = 12, uint32_t style = 0 );
    std::optional<JSObject*> CreateTooltipWithOpt( size_t optArgCount, const std::wstring& name, float pxSize, uint32_t style );
    std::optional<uint32_t> GetColourCUI( uint32_t type, const std::wstring& guidstr = L"" );
    std::optional<uint32_t> GetColourCUIWithOpt( size_t optArgCount, uint32_t type, const std::wstring& guidstr );
    std::optional<uint32_t> GetColourDUI( uint32_t type );
    std::optional<JSObject*> GetFontCUI( uint32_t type, const std::wstring& guidstr = L"" );
    std::optional<JSObject*> GetFontCUIWithOpt( size_t optArgCount, uint32_t type, const std::wstring& guidstr );
    std::optional<JSObject*> GetFontDUI( uint32_t type );
    std::optional<JS::Heap<JS::Value>> GetProperty( const std::wstring& name, JS::HandleValue defaultval = JS::NullHandleValue );
    std::optional<JS::Heap<JS::Value>> GetPropertyWithOpt( size_t optArgCount, const std::wstring& name, JS::HandleValue defaultval  );
    std::optional<std::nullptr_t> NotifyOthers( const pfc::string8_fast& name, JS::HandleValue info );
    std::optional<std::nullptr_t> Reload();
    std::optional<std::nullptr_t> Repaint( bool force = false);
    std::optional<std::nullptr_t> RepaintWithOpt( size_t optArgCount, bool force );
    std::optional<std::nullptr_t> RepaintRect( uint32_t x, uint32_t y, uint32_t w, uint32_t h, bool force = false);
    std::optional<std::nullptr_t> RepaintRectWithOpt( size_t optArgCount, uint32_t x, uint32_t y, uint32_t w, uint32_t h, bool force );
    std::optional<std::nullptr_t> SetCursor( uint32_t id );
    std::optional<uint32_t> SetInterval( JS::HandleValue func, uint32_t delay );
    std::optional<std::nullptr_t> SetProperty( const std::wstring& name, JS::HandleValue val = JS::NullHandleValue );
    std::optional<std::nullptr_t> SetPropertyWithOpt( size_t optArgCount, const std::wstring& name, JS::HandleValue val );
    std::optional<uint32_t> SetTimeout( JS::HandleValue func, uint32_t delay );
    std::optional<std::nullptr_t> ShowConfigure();
    std::optional<std::nullptr_t> ShowProperties();

public: // props
    std::optional<uint32_t> get_DlgCode();
    std::optional<uint32_t> get_Height();
    std::optional<uint32_t> get_ID();
    std::optional<uint32_t> get_InstanceType();
    std::optional<bool> get_IsTransparent();
    std::optional<bool> get_IsVisible();
    std::optional<uint32_t> get_MaxHeight();
    std::optional<uint32_t> get_MaxWidth();
    std::optional<uint32_t> get_MinHeight();
    std::optional<uint32_t> get_MinWidth();
    std::optional<pfc::string8_fast> get_Name();
    std::optional<uint32_t> get_Width();
    std::optional<std::nullptr_t> put_DlgCode( uint32_t code );
    std::optional<std::nullptr_t> put_MaxHeight( uint32_t height );
    std::optional<std::nullptr_t> put_MaxWidth( uint32_t width );
    std::optional<std::nullptr_t> put_MinHeight( uint32_t height );
    std::optional<std::nullptr_t> put_MinWidth( uint32_t width );
   
private:
    JsWindow( JSContext* cx, js_panel_window& parentPanel, std::unique_ptr<FbProperties> fbProperties );

private:
    JSContext * pJsCtx_;
    js_panel_window& parentPanel_;

    std::unique_ptr<FbProperties> fbProperties_;
};

}
