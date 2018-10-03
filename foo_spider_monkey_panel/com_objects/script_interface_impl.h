#pragma once

#include "script_interface.h"
#include "com_tools.h"
#include "helpers.h"

// NOTE: Do not use com_object_impl_t<> to initialize, use com_object_singleton_t<> instead.
class WSHUtils : public IDispatchImpl3<IWSHUtils>
{
protected:
	WSHUtils()
	{
	}
	virtual ~WSHUtils()
	{
	}

public:
	///PLUS VERSION
	STDMETHODIMP GetWndByClass(BSTR class_name, IWindow** pp);
	STDMETHODIMP GetWndByHandle(UINT window_id, IWindow** pp);
	STDMETHODIMP CloseWnd(IWindow* wnd);
	STDMETHODIMP ReleaseCapture();
};

/////PLUS VERSION
class WindowObj : public IDispatchImpl3<IWindow>
{
protected:
	UINT m_x, m_y, m_w, m_h;
	HWND m_hwnd;

	WindowObj(HWND handle) : m_hwnd(handle)
	{
	}

	virtual ~WindowObj()
	{
	}

public:
	STDMETHODIMP get_ID(UINT* pp);
	STDMETHODIMP get_ClassName(BSTR* className);
	STDMETHODIMP get_Left(INT* p);
	STDMETHODIMP get_Top(INT* p);
	STDMETHODIMP get_Width(INT* p);
	STDMETHODIMP get_Height(INT* p);
	STDMETHODIMP get_Style(INT* p);
	STDMETHODIMP get_ExStyle(INT* p);
	STDMETHODIMP get_Caption(BSTR* pp);
	STDMETHODIMP put_Left(INT l);
	STDMETHODIMP put_Top(INT t);
	STDMETHODIMP put_Width(INT w);
	STDMETHODIMP put_Height(INT h);
	STDMETHODIMP put_Style(INT s);
	STDMETHODIMP put_ExStyle(INT s);
	STDMETHODIMP put_Caption(BSTR title);

	STDMETHODIMP GetChild(BSTR caption, BSTR class_name, UINT index, IWindow** pp);
	STDMETHODIMP GetChildWithSameProcess(IWindow* searchWnd, BSTR caption, BSTR class_name, IWindow** pp);
	STDMETHODIMP GetAncestor(UINT flag, IWindow** pp);
	STDMETHODIMP SetParent(IWindow* p);
	STDMETHODIMP SendMsg(UINT msg, INT wp, INT lp);
	STDMETHODIMP Show(UINT flag);
	STDMETHODIMP Move(UINT x, UINT y, UINT w, UINT h, VARIANT_BOOL redraw);
	STDMETHODIMP IsVisible(VARIANT_BOOL* p);
	STDMETHODIMP IsMinimized(VARIANT_BOOL* p);
	STDMETHODIMP IsMaximized(VARIANT_BOOL* p);
	STDMETHODIMP ShowCaret(void);
	STDMETHODIMP SetCaretPos(INT x, INT y);
	STDMETHODIMP HideCaret(void);
	STDMETHODIMP CreateCaret(INT width, INT height);
	STDMETHODIMP DestroyCaret(void);
};
