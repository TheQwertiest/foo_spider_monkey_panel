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

	FbTooltip(HWND p_wndparent, const panel_tooltip_param_ptr & p_param_ptr);
	virtual ~FbTooltip() { }
	virtual void FinalRelease();

public:
	STDMETHODIMP Activate();
	STDMETHODIMP Deactivate();
	STDMETHODIMP GetDelayTime(int type, INT * p);
	STDMETHODIMP SetDelayTime(int type, int time);
	STDMETHODIMP SetMaxWidth(int width);
	STDMETHODIMP TrackPosition(int x, int y);
	STDMETHODIMP get_Text(BSTR * pp);
	STDMETHODIMP put_Text(BSTR text);
	STDMETHODIMP put_TrackActivate(VARIANT_BOOL activate);
};
