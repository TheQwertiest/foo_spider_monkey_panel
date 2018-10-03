#include "stdafx.h"
#include "script_interface_impl.h"
#include "popup_msg.h"
#include "stats.h"
#include "drop_source_impl.h"

#include <map>
#include <vector>
#include <algorithm>

#define TO_VARIANT_BOOL(v) ((v) ? (VARIANT_TRUE) : (VARIANT_FALSE))

STDMETHODIMP WSHUtils::GetWndByClass(BSTR class_name, IWindow** pp)
{
	if (!class_name) return E_INVALIDARG;
	if (!pp) return E_POINTER;
	IWindow *ret = NULL;
	HWND hwnd = FindWindow(class_name, NULL);
	if (hwnd != NULL)
	{
		ret = new com_object_impl_t<WindowObj>(hwnd);
	}

	*pp = ret;
	return S_OK;
}

STDMETHODIMP WSHUtils::GetWndByHandle(UINT window_id, IWindow** pp)
{
	if (!window_id) return E_INVALIDARG;
	if (!pp) return E_POINTER;
	IWindow *ret = NULL;
	HWND hwnd = reinterpret_cast<HWND>(window_id);
	if (::IsWindow(hwnd))
	{
		ret = new com_object_impl_t<WindowObj>(hwnd);
	}
	*pp = ret;
	return S_OK;
}

STDMETHODIMP WSHUtils::CloseWnd(IWindow* wnd)
{
	if (!wnd) return E_INVALIDARG;

	HWND hPWnd = NULL;
	wnd->get_ID((UINT*)&hPWnd);

	DestroyWindow(hPWnd);

	return S_OK;
}

STDMETHODIMP WSHUtils::ReleaseCapture()
{
	::ReleaseCapture();
	return S_OK;
}

STDMETHODIMP WindowObj::get_ID(UINT* pp)
{
	(*pp) = (UINT)m_hwnd;
	return S_OK;
}

STDMETHODIMP WindowObj::get_ClassName(BSTR* className)
{
	if ( !className ) return E_POINTER;

	enum
	{
		BUFFER_LEN = 1024
	};
	TCHAR buff[BUFFER_LEN] = { 0 };

	*className = NULL;

	if ( ::GetClassName( m_hwnd, buff, BUFFER_LEN ) )
	{
		( *className ) = SysAllocString( buff );
	}

	return S_OK;
}

STDMETHODIMP WindowObj::get_Left(INT * p)
{
	RECT rc;
	::GetWindowRect(m_hwnd, &rc);
	*p = rc.left;
	return S_OK;
}

STDMETHODIMP WindowObj::get_Top(INT * p)
{
	RECT rc;
	::GetWindowRect(m_hwnd, &rc);
	*p = rc.top;

	return S_OK;
}

STDMETHODIMP WindowObj::put_Left(INT l)
{
	RECT rc;
	::GetWindowRect(m_hwnd, &rc);
	::SetWindowPos(m_hwnd, NULL, l, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

	return S_OK;
}

STDMETHODIMP WindowObj::put_Top(INT t)
{
	RECT rc;
	::GetWindowRect(m_hwnd, &rc);
	::SetWindowPos(m_hwnd, NULL, rc.left, t, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	return S_OK;
}

STDMETHODIMP WindowObj::get_Width(INT * p)
{
	if (!p) return E_POINTER;

	RECT rc;
	::GetWindowRect(m_hwnd, &rc);
	*p = rc.right - rc.left;
	return S_OK;
}

STDMETHODIMP WindowObj::get_Height(INT * p)
{
	if (!p) return E_POINTER;

	RECT rc;
	::GetWindowRect(m_hwnd, &rc);
	*p = rc.bottom - rc.top;
	return S_OK;
}

STDMETHODIMP WindowObj::put_Width(INT w)
{
	RECT rc;
	::GetWindowRect(m_hwnd, &rc);
	::SetWindowPos(m_hwnd, NULL, 0, 0, w, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	return S_OK;
}

STDMETHODIMP WindowObj::put_Height(INT h)
{
	RECT rc;
	::GetWindowRect(m_hwnd, &rc);
	::SetWindowPos(m_hwnd, NULL, 0, 0, rc.right - rc.left, h, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	return S_OK;
}

STDMETHODIMP WindowObj::get_Style(INT * p)
{
	if (!p) return E_POINTER;
	*p = ::GetWindowLong(m_hwnd, GWL_STYLE);
	return S_OK;
}

STDMETHODIMP WindowObj::put_Style(INT s)
{
	::SetWindowLong(m_hwnd, GWL_STYLE, s);
	::SetWindowPos(m_hwnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
	return S_OK;
}

STDMETHODIMP WindowObj::get_ExStyle(INT * p)
{
	if (!p) return E_POINTER;
	*p = ::GetWindowLong(m_hwnd, GWL_EXSTYLE);
	return S_OK;
}

STDMETHODIMP WindowObj::put_ExStyle(INT s)
{
	::SetWindowLong(m_hwnd, GWL_EXSTYLE, s);
	::SetWindowPos(m_hwnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
	return S_OK;
}

STDMETHODIMP WindowObj::get_Caption(BSTR * pp)
{
	if (!pp) return E_POINTER;

	enum
	{
		BUFFER_LEN = 1024
	};
	TCHAR buff[BUFFER_LEN] = { 0 };

	*pp = NULL;

	if (::GetWindowText(m_hwnd, buff, BUFFER_LEN))
	{
		(*pp) = SysAllocString(buff);
	}

	return S_OK;
}

STDMETHODIMP WindowObj::put_Caption(BSTR title)
{
	if (!title)return E_INVALIDARG;
	::SetWindowText(m_hwnd, title);
	return S_OK;
}

STDMETHODIMP WindowObj::SendMsg(UINT msg, INT wp, INT lp)
{
	::SendMessage(m_hwnd, msg, (WPARAM)wp, (LPARAM)lp);
	return S_OK;
}

STDMETHODIMP WindowObj::SetParent(IWindow* p)
{
	if (!p) return E_INVALIDARG;
	HWND hPWnd = NULL;
	p->get_ID((UINT*)&hPWnd);
	::SetParent(m_hwnd, hPWnd);
	return S_OK;
}

STDMETHODIMP WindowObj::Show(UINT flag)
{
	::ShowWindow(m_hwnd, flag);
	return S_OK;
}

STDMETHODIMP WindowObj::Move(UINT x, UINT y, UINT w, UINT h, VARIANT_BOOL redraw)
{
	::MoveWindow(m_hwnd, x, y, w, h, redraw);
	return S_OK;
}

STDMETHODIMP WindowObj::IsVisible(VARIANT_BOOL * p)
{
	if (!p) return E_POINTER;
	*p = TO_VARIANT_BOOL(::IsWindowVisible(m_hwnd));
	return S_OK;
}

STDMETHODIMP WindowObj::IsMinimized(VARIANT_BOOL * p)
{
	if (!p) return E_POINTER;

	*p = TO_VARIANT_BOOL(::IsIconic(m_hwnd));
	return S_OK;
}

STDMETHODIMP WindowObj::IsMaximized(VARIANT_BOOL * p)
{
	if (!p) return E_POINTER;

	*p = TO_VARIANT_BOOL(::IsZoomed(m_hwnd));
	return S_OK;
}

STDMETHODIMP WindowObj::GetAncestor(UINT flag, IWindow** pp)
{
	if (!pp) return E_POINTER;
	IWindow *ret = NULL;
	if (flag > 0 && flag < 4)
	{
		HWND hwnd = ::GetAncestor(m_hwnd, flag);
		if (hwnd != NULL)
			ret = new com_object_impl_t<WindowObj>(hwnd);
	}
	*pp = ret;
	return S_OK;
}

STDMETHODIMP WindowObj::GetChild(BSTR caption, BSTR class_name, UINT index, IWindow** pp)
{
	if (!pp) return E_POINTER;
	if (!caption || !class_name) return E_INVALIDARG;
	if (!wcslen(caption) && !wcslen(class_name)) return E_INVALIDARG;

	IWindow *ret = NULL;

	HWND hwnd;
	HWND childHwnd = NULL;
	while (hwnd = FindWindowEx(m_hwnd, childHwnd, wcslen(class_name) ? class_name : NULL, wcslen(caption) ? caption : NULL))
	{
		childHwnd = hwnd;
		if (index == 0)
		{
			break;
		}
		--index;
	}

	if (hwnd)
	{
		ret = new com_object_impl_t<WindowObj>(hwnd);
	}

	*pp = ret;
	return S_OK;
}

STDMETHODIMP WindowObj::GetChildWithSameProcess(IWindow* searchWnd, BSTR caption, BSTR class_name, IWindow** pp)
{
	if (!pp) return E_POINTER;
	if (!caption || !class_name) return E_INVALIDARG;
	if (!wcslen(caption) && !wcslen(class_name)) return E_INVALIDARG;

	IWindow *ret = NULL;

	DWORD searchWndPid;
	HWND searchHwnd = NULL;
	searchWnd->get_ID((UINT*)&searchHwnd);
	::GetWindowThreadProcessId(searchHwnd, &searchWndPid);

	HWND hwnd;
	HWND childHwnd = NULL;
	while (hwnd = FindWindowEx(m_hwnd, childHwnd, wcslen(class_name) ? class_name : NULL, wcslen(caption) ? caption : NULL))
	{
		childHwnd = hwnd;
		DWORD wndPid;
		::GetWindowThreadProcessId(hwnd, &wndPid);
		if (wndPid == searchWndPid)
		{
			break;
		}
	}

	if (hwnd)
	{
		ret = new com_object_impl_t<WindowObj>(hwnd);
	}

	*pp = ret;
	return S_OK;
}

STDMETHODIMP WindowObj::ShowCaret(void)
{
	::ShowCaret(m_hwnd);
	return S_OK;
}

STDMETHODIMP WindowObj::SetCaretPos(INT x, INT y)
{
	::SetCaretPos(x, y);
	return S_OK;
}

STDMETHODIMP WindowObj::HideCaret(void)
{
	::HideCaret(m_hwnd);
	return S_OK;
}

STDMETHODIMP WindowObj::DestroyCaret(void)
{
	::DestroyCaret();
	return S_OK;
}

STDMETHODIMP WindowObj::CreateCaret(INT width, INT height)
{
	::CreateCaret(m_hwnd, NULL, width, height);
	return S_OK;
}
