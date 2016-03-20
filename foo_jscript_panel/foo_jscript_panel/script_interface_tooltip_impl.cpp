#include "stdafx.h"
#include "script_interface_impl.h"
#include "script_interface_tooltip_impl.h"
#include "helpers.h"
#include "com_array.h"
#include "panel_manager.h"


FbTooltip::FbTooltip(HWND p_wndparent, const panel_tooltip_param_ptr & p_param_ptr) 
	: m_wndparent(p_wndparent)
	, m_panel_tooltip_param_ptr(p_param_ptr)
	, m_tip_buffer(SysAllocString(PFC_WIDESTRING(JSP_NAME)))
{
	m_wndtooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
		WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
		CW_USEDEFAULT, CW_USEDEFAULT, 
		CW_USEDEFAULT, CW_USEDEFAULT,
		p_wndparent, NULL, core_api::get_my_instance(), NULL);

	// Original position
	SetWindowPos(m_wndtooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

	// Set up tooltip information.
	memset(&m_ti, 0, sizeof(m_ti));

	m_ti.cbSize = sizeof(m_ti);
	m_ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS | TTF_TRANSPARENT;
	m_ti.hinst = core_api::get_my_instance();
	m_ti.hwnd = p_wndparent;
	m_ti.uId = (UINT_PTR)p_wndparent;
	m_ti.lpszText = m_tip_buffer;

	HFONT font = CreateFont(
		-(INT)m_panel_tooltip_param_ptr->font_size,
		0,
		0, 
		0, 
		(m_panel_tooltip_param_ptr->font_style & Gdiplus::FontStyleBold) ? FW_BOLD : FW_NORMAL,
		(m_panel_tooltip_param_ptr->font_style & Gdiplus::FontStyleItalic) ? TRUE : FALSE,
		(m_panel_tooltip_param_ptr->font_style & Gdiplus::FontStyleUnderline) ? TRUE : FALSE,
		(m_panel_tooltip_param_ptr->font_style & Gdiplus::FontStyleStrikeout) ? TRUE : FALSE,
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE,
		m_panel_tooltip_param_ptr->font_name);

	SendMessage(m_wndtooltip, TTM_ADDTOOL, 0, (LPARAM)&m_ti);	
	SendMessage(m_wndtooltip, TTM_ACTIVATE, FALSE, 0);
	SendMessage(m_wndtooltip, WM_SETFONT, (WPARAM)font, MAKELPARAM(FALSE, 0));

	m_panel_tooltip_param_ptr->tooltip_hwnd = m_wndtooltip;
	m_panel_tooltip_param_ptr->tooltip_size.cx = -1;
	m_panel_tooltip_param_ptr->tooltip_size.cy = -1;
}

void FbTooltip::FinalRelease()
{
	if (m_wndtooltip && IsWindow(m_wndtooltip))
	{
		DestroyWindow(m_wndtooltip);
		m_wndtooltip = NULL;
	}

	if (m_tip_buffer)
	{
		SysFreeString(m_tip_buffer);
		m_tip_buffer = NULL;
	}
}

STDMETHODIMP FbTooltip::get_Text(BSTR * pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;

	(*pp) = SysAllocString(m_tip_buffer);
	return S_OK;
}

STDMETHODIMP FbTooltip::put_Text(BSTR text)
{
	TRACK_FUNCTION();

	if (!text) return E_INVALIDARG;

	// realloc string
	SysReAllocString(&m_tip_buffer, text);
	m_ti.lpszText = m_tip_buffer;
	SendMessage(m_wndtooltip, TTM_SETTOOLINFO, 0, (LPARAM)&m_ti);
	return S_OK;
}

STDMETHODIMP FbTooltip::put_TrackActivate(VARIANT_BOOL activate)
{
	TRACK_FUNCTION();

	if (activate) {
		m_ti.uFlags |= TTF_TRACK | TTF_ABSOLUTE;
	} else {
		m_ti.uFlags &= ~(TTF_TRACK | TTF_ABSOLUTE);
	}

	SendMessage(m_wndtooltip, TTM_TRACKACTIVATE, activate ? TRUE : FALSE, (LPARAM)&m_ti);
	return S_OK;
}

STDMETHODIMP FbTooltip::Activate()
{
	TRACK_FUNCTION();

	SendMessage(m_wndtooltip, TTM_ACTIVATE, TRUE, 0);
	return S_OK;
}

STDMETHODIMP FbTooltip::Deactivate()
{
	TRACK_FUNCTION();

	SendMessage(m_wndtooltip, TTM_ACTIVATE, FALSE, 0);
	return S_OK;
}

STDMETHODIMP FbTooltip::SetMaxWidth(int width)
{
	TRACK_FUNCTION();

	SendMessage(m_wndtooltip, TTM_SETMAXTIPWIDTH, 0, width);
	return S_OK;
}

STDMETHODIMP FbTooltip::GetDelayTime(int type, INT * p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;
	if (type < TTDT_AUTOMATIC || type > TTDT_INITIAL) return E_INVALIDARG;

	*p = SendMessage(m_wndtooltip, TTM_GETDELAYTIME, type, 0);
	return S_OK;
}

STDMETHODIMP FbTooltip::SetDelayTime(int type, int time)
{
	TRACK_FUNCTION();

	if (type < TTDT_AUTOMATIC || type > TTDT_INITIAL) return E_INVALIDARG;

	SendMessage(m_wndtooltip, TTM_SETDELAYTIME, type, time);
	return S_OK;
}

STDMETHODIMP FbTooltip::TrackPosition(int x, int y)
{
	TRACK_FUNCTION();

	POINT pt = { x, y };
	ClientToScreen(m_wndparent, &pt);
	SendMessage(m_wndtooltip, TTM_TRACKPOSITION, 0, MAKELONG(pt.x, pt.y));
	return S_OK;
}
