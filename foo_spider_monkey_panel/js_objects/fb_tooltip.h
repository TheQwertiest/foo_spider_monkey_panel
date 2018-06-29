#pragma once

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

/*

struct panel_tooltip_param
{
HWND tooltip_hwnd;
SIZE tooltip_size;
BSTR font_name;
float font_size;
int font_style;

panel_tooltip_param() : tooltip_hwnd(0)
{
}
};

typedef std::shared_ptr<panel_tooltip_param> panel_tooltip_param_ptr;

class FbTooltip : public IDisposableImpl4<IFbTooltip>
{
protected:
HWND m_wndtooltip;
HWND m_wndparent;
BSTR m_tip_buffer;
TOOLINFO m_ti;
panel_tooltip_param_ptr m_panel_tooltip_param_ptr;

FbTooltip(HWND p_wndparent, const panel_tooltip_param_ptr& p_param_ptr);
virtual ~FbTooltip();
virtual void FinalRelease();

public:
STDMETHODIMP Activate();
STDMETHODIMP Deactivate();
STDMETHODIMP GetDelayTime(int type, int* p);
STDMETHODIMP SetDelayTime(int type, int time);
STDMETHODIMP SetMaxWidth(int width);
STDMETHODIMP TrackPosition(int x, int y);
STDMETHODIMP get_Text(BSTR* pp);
STDMETHODIMP put_Text(BSTR text);
STDMETHODIMP put_TrackActivate(VARIANT_BOOL activate);
};

*/
}
