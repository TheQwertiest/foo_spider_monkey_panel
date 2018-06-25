#pragma once

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;

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
    
    static JSObject* Create( JSContext* cx );

    static const JSClass& GetClass();

public: // props
    //std::optional<uint32_t> Height() const;;

private:
    JsWindow( JSContext* cxt );
    JsWindow( const JsWindow& ) = delete;

private:
    JSContext * pJsCtx_;
};

}
