#ifndef __PROPERTYITEMEDITORS__H
#define __PROPERTYITEMEDITORS__H

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CPropertyItemEditors - Editors for Property controls
//
// Written by Bjarke Viksoe (bjarke@viksoe.dk)
// Copyright (c) 2001-2003 Bjarke Viksoe.
//
// This code may be used in compiled form in any way you desire. This
// source file may be redistributed by any means PROVIDING it is
// not sold for profit without the authors written consent, and
// providing that this notice and the authors name is included.
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability if it causes any damage to you or your
// computer whatsoever. It's free, so don't hassle me about it.
//
// Beware of bugs.
//

#ifndef __PROPERTYITEM__H
#error PropertyItemEditors.h requires PropertyItem.h to be included first
#endif

#define PROP_TEXT_INDENT 2


/////////////////////////////////////////////////////////////////////////////
// Plain editor with a EDIT box

class CPropertyEditWindow :
	public CWindowImpl< CPropertyEditWindow, CEdit, CControlWinTraits >
{
public:
	DECLARE_WND_SUPERCLASS(_T("WTL_InplacePropertyEdit"), CEdit::GetWndClassName())

	bool m_fCancel;

	CPropertyEditWindow() : m_fCancel(false)
	{
	}

	virtual void OnFinalMessage(HWND /*hWnd*/)
	{
		delete this;
	}

	BEGIN_MSG_MAP(CPropertyEditWindow)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
		MESSAGE_HANDLER(WM_CHAR, OnChar)
		MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
		MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
		MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDlgCode)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		LRESULT lRes = DefWindowProc();
		SetFont(CWindow(GetParent()).GetFont());
		SetMargins(PROP_TEXT_INDENT, 0);   // Force EDIT margins so text doesn't jump
		return lRes;
	}
	LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
		switch (wParam)
		{
		case VK_ESCAPE:
			m_fCancel = true;
			// FALL THROUGH...
		case VK_RETURN:
			// Force focus to parent to update value (see OnKillFocus()...)
			::SetFocus(GetParent());
			// FIX: Allowing a multiline EDIT to VK_ESCAPE will send a WM_CLOSE
			//      to the list control if it's embedded in a dialog!?
			return 0;
		case VK_TAB:
		case VK_UP:
		case VK_DOWN:
			return ::PostMessage(GetParent(), WM_USER_PROP_NAVIGATE, LOWORD(wParam), 0);
		case VK_LEFT:
			int lLow, lHigh;
			GetSel(lLow, lHigh);
			if (lLow != lHigh || lLow != 0) break;
			return ::PostMessage(GetParent(), WM_USER_PROP_NAVIGATE, LOWORD(wParam), 0);
		case VK_RIGHT:
			GetSel(lLow, lHigh);
			if (lLow != lHigh || lLow != GetWindowTextLength()) break;
			return ::PostMessage(GetParent(), WM_USER_PROP_NAVIGATE, LOWORD(wParam), 0);
		}
		bHandled = FALSE;
		return 0;
	}
	LRESULT OnChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
		switch (LOWORD(wParam))
		{
		case VK_RETURN:
		case VK_ESCAPE:
			// Do not BEEP!!!!
			return 0;
		}
		bHandled = FALSE;
		return 0;
	}
	LRESULT OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		m_fCancel = false;
		bHandled = FALSE;
		return 0;
	}
	LRESULT OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		LRESULT lRes = DefWindowProc(uMsg, wParam, lParam);
		m_fCancel |= (GetModify() == FALSE);
		::SendMessage(GetParent(), m_fCancel ? WM_USER_PROP_CANCELPROPERTY : WM_USER_PROP_UPDATEPROPERTY, 0, (LPARAM) m_hWnd);
		return lRes;
	}
	LRESULT OnGetDlgCode(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		return DefWindowProc(uMsg, wParam, lParam) | DLGC_WANTALLKEYS | DLGC_WANTARROWS;
	}
};


/////////////////////////////////////////////////////////////////////////////
// General implementation of editor with button

template < class T, class TBase = CEdit >
class CPropertyDropWindowImpl :
	public CWindowImpl< T, TBase, CControlWinTraits >,
	public CThemeImpl< CPropertyDropWindowImpl< T, TBase > >
{
public:
	DECLARE_WND_SUPERCLASS(NULL, TBase::GetWndClassName())

	CContainedWindowT<CButton> m_wndButton;
	bool m_bReadOnly;

	virtual void OnFinalMessage(HWND /*hWnd*/)
	{
		delete(T*) this;
	}

	typedef CPropertyDropWindowImpl< T > thisClass;
	typedef CThemeImpl< CPropertyDropWindowImpl< T, TBase > > themeClass;

	BEGIN_MSG_MAP(thisClass)
		CHAIN_MSG_MAP(themeClass)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
		MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
		MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
		MESSAGE_HANDLER(WM_CHAR, OnChar)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, OnMouseButtonClick)
		MESSAGE_HANDLER(WM_RBUTTONDOWN, OnMouseButtonClick)
		MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDlgCode)
		REFLECT_NOTIFICATIONS()
		ALT_MSG_MAP(1) // Button
		MESSAGE_HANDLER(WM_KEYDOWN, OnButtonKeyDown)
		MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDlgCode)
	END_MSG_MAP()

	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		LRESULT lRes = DefWindowProc(uMsg, wParam, lParam);
		RECT rcClient = { 0 };
		GetClientRect(&rcClient);
		int cy = rcClient.bottom - rcClient.top;
		// Setup EDIT control
		SetFont(CWindow(GetParent()).GetFont());
		ModifyStyle(WS_BORDER, ES_LEFT);
		SendMessage(EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(PROP_TEXT_INDENT, ::GetSystemMetrics(SM_CXVSCROLL)));
		// Create button
		RECT rcButton = { rcClient.right - cy, rcClient.top - 1, rcClient.right, rcClient.bottom };
		m_wndButton.Create(this, 1, m_hWnd, &rcButton, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | BS_PUSHBUTTON | BS_OWNERDRAW);
		ATLASSERT(m_wndButton.IsWindow());
		m_wndButton.SetFont(GetFont());
		// HACK: Windows needs to repaint this guy again!
		m_wndButton.SetFocus();
		m_bReadOnly = true;
		return lRes;
	}
	LRESULT OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if (!m_bReadOnly)
		{
			bHandled = FALSE;
		}
		else
		{
			// Set focus to button to prevent input
			m_wndButton.SetFocus();
			m_wndButton.Invalidate();
		}
		return 0;
	}
	LRESULT OnKillFocus(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if ((HWND)wParam != m_wndButton) ::SendMessage(GetParent(), WM_USER_PROP_UPDATEPROPERTY, 0, (LPARAM)m_hWnd);
		bHandled = FALSE;
		return 0;
	}
	LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if (m_bReadOnly)
		{
			bHandled = FALSE;
			return 0;
		}
		switch (wParam)
		{
		case VK_F2:
		case VK_F4:
		case VK_SPACE:
			m_wndButton.Click();
			return 0;
		case VK_RETURN:
		case VK_ESCAPE:
			// Announce the new value
			::PostMessage(GetParent(), wParam == VK_RETURN ? WM_USER_PROP_UPDATEPROPERTY : WM_USER_PROP_CANCELPROPERTY, 0, (LPARAM) m_hWnd);
			::SetFocus(GetParent());
			break;
		case VK_TAB:
		case VK_UP:
		case VK_DOWN:
			return ::PostMessage(GetParent(), WM_USER_PROP_NAVIGATE, LOWORD(wParam), 0);
		case VK_LEFT:
			int lLow, lHigh;
			SendMessage(EM_GETSEL, (WPARAM) &lLow, (LPARAM) &lHigh);
			if (lLow != lHigh || lLow != 0) break;
			return ::PostMessage(GetParent(), WM_USER_PROP_NAVIGATE, LOWORD(wParam), 0);
		case VK_RIGHT:
			SendMessage(EM_GETSEL, (WPARAM) &lLow, (LPARAM) &lHigh);
			if (lLow != lHigh || lLow != GetWindowTextLength()) break;
			return ::PostMessage(GetParent(), WM_USER_PROP_NAVIGATE, LOWORD(wParam), 0);
		}
		bHandled = FALSE;
		return 0;
	}
	LRESULT OnChar(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		// Don't allow any editing
		if (!m_bReadOnly) bHandled = FALSE;
		return 0;
	}
	LRESULT OnMouseButtonClick(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		// Don't allow selection or context menu for edit box
		if (!m_bReadOnly) bHandled = FALSE;
		return 0;
	}

	// Button

	LRESULT OnButtonKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
		switch (wParam)
		{
		case VK_UP:
		case VK_DOWN:
			return ::PostMessage(GetParent(), WM_USER_PROP_NAVIGATE, LOWORD(wParam), 0);
		case VK_F2:
		case VK_F4:
		case VK_SPACE:
			m_wndButton.Click();
			return 0;
		case VK_ESCAPE:
			::PostMessage(GetParent(), WM_USER_PROP_UPDATEPROPERTY, 0, (LPARAM) m_hWnd);
			return 0;
		}
		bHandled = FALSE;
		return 0;
	}
	LRESULT OnGetDlgCode(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		return DefWindowProc(uMsg, wParam, lParam) | DLGC_WANTALLKEYS;
	}
};


/////////////////////////////////////////////////////////////////////////////
// Editor with dropdown list

class CPropertyListWindow :
	public CPropertyDropWindowImpl<CPropertyListWindow>
{
public:
	DECLARE_WND_SUPERCLASS(_T("WTL_InplacePropertyList"), CEdit::GetWndClassName())

	CContainedWindowT<CListBox> m_wndList;
	int m_cyList;      // Used to resize the listbox when first shown

	typedef CPropertyDropWindowImpl<CPropertyListWindow> baseClass;

	CPropertyListWindow()
	{
		SetThemeClassList(L"COMBOBOX");
	}

	BEGIN_MSG_MAP(CPropertyListWindow)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_CHAR, OnChar)
		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
		COMMAND_CODE_HANDLER(BN_CLICKED, OnButtonClicked)
		CHAIN_MSG_MAP(baseClass)
		ALT_MSG_MAP(1) // Button
		CHAIN_MSG_MAP_ALT(baseClass, 1)
		ALT_MSG_MAP(2) // List
		MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
		MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
		MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
	END_MSG_MAP()

	void AddItem(LPCTSTR pstrItem)
	{
		ATLASSERT(m_wndList.IsWindow());
		ATLASSERT(!::IsBadStringPtr(pstrItem, -1));
		m_wndList.AddString(pstrItem);
		m_cyList = 0;
	}
	void SelectItem(int idx)
	{
		ATLASSERT(m_wndList.IsWindow());
		m_wndList.SetCurSel(idx);
	}

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		// Create dropdown list (as hidden)
		RECT rc = CWindow::rcDefault;
		m_wndList.Create(this, 2, m_hWnd, &rc, NULL, WS_POPUP | WS_BORDER | WS_VSCROLL);
		ATLASSERT(m_wndList.IsWindow());
		m_wndList.SetFont(CWindow(GetParent()).GetFont());
		// Go create the rest of the control...
		bHandled = FALSE;
		return 0;
	}
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if (m_wndList.IsWindow()) m_wndList.DestroyWindow();
		bHandled = FALSE;
		return 0;
	}
	LRESULT OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		// Let the dropdown-box handle the keypress...
		if ((m_wndList.GetStyle() & WS_VISIBLE) != 0)
		{
			m_wndList.PostMessage(uMsg, wParam, lParam);
		}
		else
		{
			TCHAR szStr[] = {(TCHAR) wParam, _T('\0') };
			int idx = m_wndList.FindString(-1, szStr);
			if (idx == LB_ERR) return 0;
			m_wndList.SetCurSel(idx);
			BOOL bDummy = FALSE;
			OnKeyDown(WM_KEYDOWN, VK_RETURN, 0, bDummy);
		}
		return 0; // Don't allow any editing
	}
	LRESULT OnButtonClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if (m_cyList == 0)
		{
			// Resize list to fit all items (but not more than 140 pixels)
			const int MAX_HEIGHT = 140;
			int cy = m_wndList.GetCount() * m_wndList.GetItemHeight(0);
			m_cyList = min(MAX_HEIGHT, cy + (::GetSystemMetrics(SM_CYBORDER) * 2));
		}
		// Move the dropdown under the item
		RECT rcWin = { 0 };
		GetWindowRect(&rcWin);
		RECT rc = { rcWin.left, rcWin.bottom, rcWin.right, rcWin.bottom + m_cyList };
		m_wndList.SetWindowPos(HWND_TOPMOST, &rc, SWP_SHOWWINDOW);
		return 0;
	}

	// List message handlers

	LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
		switch (wParam)
		{
		case VK_RETURN:
			{
				int idx = m_wndList.GetCurSel();
				if (idx >= 0)
				{
					// Copy text from list to item
					CString text;
					m_wndList.GetText(idx, text);
					SetWindowText(text);
					// Announce the new value
					::SendMessage(GetParent(), WM_USER_PROP_UPDATEPROPERTY, 0, (LPARAM) m_hWnd);
				}
			}
			::SetFocus(GetParent());
			break;
		case VK_ESCAPE:
			// Announce the cancellation
			::SendMessage(GetParent(), WM_USER_PROP_CANCELPROPERTY, 0, (LPARAM) m_hWnd);
			::SetFocus(GetParent());
			break;
		}
		bHandled = FALSE;
		return 0;
	}
	LRESULT OnLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		LRESULT lRes = m_wndList.DefWindowProc();
		// Selected an item? Fake RETURN key to copy new value...
		BOOL bDummy = FALSE;
		OnKeyDown(WM_KEYDOWN, VK_RETURN, 0, bDummy);
		return lRes;
	}
	LRESULT OnKillFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		LRESULT lRes = m_wndList.DefWindowProc();
		m_wndList.ShowWindow(SW_HIDE);
		return lRes;
	}

	// Ownerdrawn button message handler
	LRESULT OnDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
	{
		LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT) lParam;
		if (m_wndButton != lpdis->hwndItem) return 0;

		// Paint as dropdown button
		if (m_hTheme)
		{
			RECT rc;

			CopyRect(&rc, &lpdis->rcItem);
			rc.top += 1;
			//rc.bottom -= 1;
			::DrawThemeParentBackground(lpdis->hwndItem, lpdis->hDC, &rc);
			DrawThemeBackground(lpdis->hDC, CP_DROPDOWNBUTTON, (lpdis->itemState & ODS_SELECTED) != 0 ? CBXS_PRESSED : CBXS_NORMAL, &rc, NULL);
			DrawThemeEdge(lpdis->hDC, CP_DROPDOWNBUTTON, (lpdis->itemState & ODS_SELECTED) != 0 ? CBXS_PRESSED : CBXS_NORMAL, &rc, 0, 0, NULL);
		}
		else
		{
			DrawFrameControl(lpdis->hDC, &lpdis->rcItem, DFC_SCROLL, (lpdis->itemState & ODS_SELECTED) != 0 ? DFCS_SCROLLDOWN | DFCS_PUSHED : DFCS_SCROLLDOWN);
		}

		return 0;
	}
};

#endif // __PROPERTYITEMEDITORS__H
