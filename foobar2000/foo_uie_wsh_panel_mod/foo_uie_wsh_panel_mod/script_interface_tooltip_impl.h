#pragma once

#include "script_interface_tooltip.h"
#include "com_tools.h"
#include "panel_tooltip_param.h"


class FbTooltip : public IDisposableImpl4<IFbTooltip>
{
protected:
	HWND m_wndtooltip;
	HWND m_wndparent;
	BSTR m_tip_buffer;
	TOOLINFO m_ti;
	panel_tooltip_param_ptr m_panel_tooltip_param_ptr;

	FbTooltip(HWND p_wndparent, bool p_no_background, const panel_tooltip_param_ptr & p_param_ptr);
	virtual ~FbTooltip() { }
	virtual void FinalRelease();

public:
	STDMETHODIMP get_Text(BSTR * pp);
	STDMETHODIMP put_Text(BSTR text);
	STDMETHODIMP put_TrackActivate(VARIANT_BOOL activate);
	STDMETHODIMP get_Width(int * outWidth);
	STDMETHODIMP put_Width(int width);
	STDMETHODIMP get_Height(int * outHeight);
	STDMETHODIMP put_Height(int height);
	STDMETHODIMP Activate();
	STDMETHODIMP Deactivate();
	STDMETHODIMP SetMaxWidth(int width);
	STDMETHODIMP GetDelayTime(int type, INT * p);
	STDMETHODIMP SetDelayTime(int type, int time);
	STDMETHODIMP TrackPosition(int x, int y);
};
