#pragma once

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

class JsWindow
{
public:
    ~JsWindow();
    
    static JSObject* Create( JSContext* cx, js_panel_window& parentPanel );

    static const JSClass& GetClass();

    void RemoveHeapTracer();

public: // methods
    std::optional<std::nullptr_t> ClearInterval( uint32_t intervalId );
    std::optional<std::nullptr_t> ClearTimeout( uint32_t timeoutId );
    std::optional<JSObject*> CreatePopupMenu();
    std::optional<JSObject*> CreateThemeManager( const std::wstring& classid );
    std::optional<JSObject*> CreateTooltip( const std::wstring& name, uint32_t pxSize, uint32_t style );
    std::optional<uint32_t> GetColourCUI( uint32_t type, const std::wstring& guidstr );
    std::optional<uint32_t> GetColourDUI( uint32_t type );
    std::optional<JSObject*> GetFontCUI( uint32_t type, const std::wstring& guidstr );
    std::optional<JSObject*> GetFontDUI( uint32_t type );
    std::optional<JS::Value*> GetProperty( const std::string& name, JS::HandleValue defaultval );
    std::optional<std::nullptr_t> NotifyOthers( const std::string& name, JS::HandleValue info );
    std::optional<std::nullptr_t> Reload();
    std::optional<std::nullptr_t> Repaint( bool force );
    std::optional<std::nullptr_t> RepaintRect( uint32_t x, uint32_t y, uint32_t w, uint32_t h, bool force );
    std::optional<std::nullptr_t> SetCursor( uint8_t id );
    std::optional<uint32_t> SetInterval( JS::HandleFunction func, uint32_t delay );
    std::optional<std::nullptr_t> SetProperty( const std::string& name, JS::HandleValue val );
    std::optional<uint32_t> SetTimeout( JS::HandleFunction func, uint32_t delay );
    std::optional<std::nullptr_t> ShowConfigure();
    std::optional<std::nullptr_t> ShowProperties();

public: // props
    std::optional<uint32_t> get_DlgCode();
    std::optional<uint32_t> get_Height();
    std::optional<uint64_t> get_Id();
    std::optional<uint32_t> get_InstanceType();
    std::optional<bool> get_IsTransparent();
    std::optional<bool> get_IsVisible();
    std::optional<uint32_t> get_MaxHeight();
    std::optional<uint32_t> get_MaxWidth();
    std::optional<uint32_t> get_MinHeight();
    std::optional<uint32_t> get_MinWidth();
    std::optional<std::string> get_Name();
    std::optional<uint32_t> get_Width();
    std::optional<std::nullptr_t> put_DlgCode( uint32_t code );
    std::optional<std::nullptr_t> put_MaxHeight( uint32_t height );
    std::optional<std::nullptr_t> put_MaxWidth( uint32_t width );
    std::optional<std::nullptr_t> put_MinHeight( uint32_t height );
    std::optional<std::nullptr_t> put_MinWidth( uint32_t width );
   
private:
    JsWindow( JSContext* cx, js_panel_window& parentPanel );
    JsWindow( const JsWindow& ) = delete;
    JsWindow& operator=( const JsWindow& ) = delete;

private:
    JSContext * pJsCtx_;
    js_panel_window& parentPanel_;

    JS::PersistentRootedObject jsFbProperties_;
    JsFbProperties* pFbProperties_ = nullptr;
};

}
