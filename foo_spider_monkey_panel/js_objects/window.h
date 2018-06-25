#pragma once

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;

class js_panel_window;

namespace mozjs
{

/*

STDMETHODIMP ClearInterval(UINT intervalID);
STDMETHODIMP ClearTimeout(UINT timeoutID);
STDMETHODIMP CreatePopupMenu(IMenuObj** pp);
STDMETHODIMP CreateThemeManager(BSTR classid, IThemeManager** pp);
STDMETHODIMP CreateTooltip(BSTR name, float pxSize, int style, IFbTooltip** pp);
STDMETHODIMP GetColourCUI(UINT type, BSTR guidstr, int* p);
STDMETHODIMP GetColourDUI(UINT type, int* p);
STDMETHODIMP GetFontCUI(UINT type, BSTR guidstr, IGdiFont** pp);
STDMETHODIMP GetFontDUI(UINT type, IGdiFont** pp);
STDMETHODIMP GetProperty(BSTR name, VARIANT defaultval, VARIANT* p);
STDMETHODIMP NotifyOthers(BSTR name, VARIANT info);
STDMETHODIMP Reload();
STDMETHODIMP Repaint(VARIANT_BOOL force);
STDMETHODIMP RepaintRect(LONG x, LONG y, LONG w, LONG h, VARIANT_BOOL force);
STDMETHODIMP SetCursor(UINT id);
STDMETHODIMP SetInterval(IDispatch* func, int delay, UINT* outIntervalID);
STDMETHODIMP SetProperty(BSTR name, VARIANT val);
STDMETHODIMP SetTimeout(IDispatch* func, int delay, UINT* outTimeoutID);
STDMETHODIMP ShowConfigure();
STDMETHODIMP ShowProperties();
STDMETHODIMP get_DlgCode(UINT* p);
STDMETHODIMP get_Height(INT* p);
STDMETHODIMP get_ID(UINT* p);
STDMETHODIMP get_InstanceType(UINT* p);
STDMETHODIMP get_IsTransparent(VARIANT_BOOL* p);
STDMETHODIMP get_IsVisible(VARIANT_BOOL* p);
STDMETHODIMP get_MaxHeight(UINT* p);
STDMETHODIMP get_MaxWidth(UINT* p);
STDMETHODIMP get_MinHeight(UINT* p);
STDMETHODIMP get_MinWidth(UINT* p);
STDMETHODIMP get_Name(BSTR* p);
STDMETHODIMP get_Width(INT* p);
STDMETHODIMP put_DlgCode(UINT code);
STDMETHODIMP put_MaxHeight(UINT height);
STDMETHODIMP put_MaxWidth(UINT width);
STDMETHODIMP put_MinHeight(UINT height);
STDMETHODIMP put_MinWidth(UINT width);

*/

class JsWindow
{
public:
    ~JsWindow();
    
    static JSObject* Create( JSContext* cx, js_panel_window& parentPanel );

    static const JSClass& GetClass();

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
    std::optional<std::string> get_Name();
    std::optional<uint32_t> get_Width();
    std::optional<std::nullptr_t> put_DlgCode( uint32_t code );
    std::optional<std::nullptr_t> put_MaxHeight( uint32_t height );
    std::optional<std::nullptr_t> put_MaxWidth( uint32_t width );
    std::optional<std::nullptr_t> put_MinHeight( uint32_t height );
    std::optional<std::nullptr_t> put_MinWidth( uint32_t width );

public: // methods
    std::optional<std::nullptr_t> ClearInterval( uint32_t intervalID );
    std::optional<std::nullptr_t> ClearTimeout( uint32_t timeoutID );
    //std::optional<std::nullptr_t> CreatePopupMenu( IMenuObj** pp );
    //std::optional<std::nullptr_t> CreateThemeManager( BSTR classid, IThemeManager** pp );
    //std::optional<std::nullptr_t> CreateTooltip( BSTR name, float pxSize, int style, IFbTooltip** pp );
    std::optional<std::uint32_t> GetColourCUI( uint32_t type, std::wstring guidstr );
    std::optional<std::uint32_t> GetColourDUI( uint32_t type );
    std::optional<JSObject*> GetFontCUI( uint32_t type, std::wstring guidstr );
    std::optional<JSObject*> GetFontDUI( uint32_t type );
    //std::optional<std::nullptr_t> GetProperty( BSTR name, VARIANT defaultval, VARIANT* p );
    //std::optional<std::nullptr_t> NotifyOthers( BSTR name, VARIANT info );
    std::optional<std::nullptr_t> Reload();
    std::optional<std::nullptr_t> Repaint( bool force );
    std::optional<std::nullptr_t> RepaintRect( uint32_t x, uint32_t y, uint32_t w, uint32_t h, bool force );
    std::optional<std::nullptr_t> SetCursor( uint32_t id );
    //std::optional<std::nullptr_t> SetInterval( IDispatch* func, int delay, UINT* outIntervalID );
    //std::optional<std::nullptr_t> SetProperty( BSTR name, VARIANT val );
    //std::optional<std::nullptr_t> SetTimeout( IDispatch* func, int delay, UINT* outTimeoutID );
    std::optional<std::nullptr_t> ShowConfigure();
    std::optional<std::nullptr_t> ShowProperties();
   
private:
    JsWindow( JSContext* cx, js_panel_window& parentPanel );
    JsWindow( const JsWindow& ) = delete;

private:
    JSContext * pJsCtx_;
    js_panel_window& parentPanel_;
};

}
