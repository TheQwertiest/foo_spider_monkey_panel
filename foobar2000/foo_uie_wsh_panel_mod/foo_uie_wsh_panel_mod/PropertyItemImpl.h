#ifndef __PROPERTYITEMIMPL__H
#define __PROPERTYITEMIMPL__H

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CPropertyItemImpl - Property implementations for the Property controls
//
// Written by Bjarke Viksoe (bjarke@viksoe.dk)
// Copyright (c) 2001-2002 Bjarke Viksoe.
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
#error PropertyItemImpl.h requires PropertyItem.h to be included first
#endif

#ifndef __PROPERTYITEMEDITORS__H
#error PropertyItemImpl.h requires PropertyItemEditors.h to be included first
#endif

#ifndef __ATLBASE_H__
#error PropertyItem.h requires atlbase.h to be included first
#endif



/////////////////////////////////////////////////////////////////////////////
// Base CProperty class

class CProperty : public IProperty
{
protected:
	HWND   m_hWndOwner;
	LPTSTR m_pszName;
	bool   m_fEnabled;
	LPARAM m_lParam;

public:
	CProperty(LPCTSTR pstrName, LPARAM lParam) : m_fEnabled(true), m_lParam(lParam), m_hWndOwner(NULL)
	{
		ATLASSERT(!::IsBadStringPtr(pstrName, -1));
		int size = (::lstrlen(pstrName) * sizeof(TCHAR)) + 1;
		ATLTRY(m_pszName = new TCHAR[size]);
		ATLASSERT(m_pszName);
		::lstrcpyn(m_pszName, pstrName, size);
	}
	virtual ~CProperty()
	{
		delete [] m_pszName;
	}
	virtual void SetOwner(HWND hWnd, LPVOID /*pData*/)
	{
		ATLASSERT(::IsWindow(hWnd));
		ATLASSERT(m_hWndOwner == NULL); // Cannot set it twice
		m_hWndOwner = hWnd;
	}
	virtual LPCTSTR GetName() const
	{
		return m_pszName; // Dangerous!
	}
	virtual void SetEnabled(BOOL bEnable)
	{
		m_fEnabled = (bEnable == TRUE);
	}
	virtual BOOL IsEnabled() const
	{
		return m_fEnabled;
	}
	virtual void SetItemData(LPARAM lParam)
	{
		m_lParam = lParam;
	}
	virtual LPARAM GetItemData() const
	{
		return m_lParam;
	}
	virtual void DrawName(PROPERTYDRAWINFO& di)
	{
		CDCHandle dc(di.hDC);
		COLORREF clrBack, clrFront;
		if ((di.state & ODS_DISABLED) != 0)
		{
			clrFront = di.clrDisabled;
			clrBack = di.clrBack;
		}
		else if ((di.state & ODS_SELECTED) != 0)
		{
			clrFront = di.clrSelText;
			clrBack = di.clrSelBack;
		}
		else
		{
			clrFront = di.clrText;
			clrBack = di.clrBack;
		}
		RECT rcItem = di.rcItem;
		dc.FillSolidRect(&rcItem, clrBack);
		rcItem.left += 2; // Indent text
		dc.SetBkMode(TRANSPARENT);
		dc.SetBkColor(clrBack);
		dc.SetTextColor(clrFront);
		dc.DrawText(m_pszName, -1, &rcItem, DT_LEFT | DT_SINGLELINE | DT_EDITCONTROL | DT_NOPREFIX | DT_VCENTER);
	}
	virtual void DrawValue(PROPERTYDRAWINFO& /*di*/)
	{
	}
	virtual HWND CreateInplaceControl(HWND /*hWnd*/, const RECT& /*rc*/)
	{
		return NULL;
	}
	virtual BOOL Activate(UINT /*action*/, LPARAM /*lParam*/)
	{
		return TRUE;
	}
	virtual BOOL GetDisplayValue(LPTSTR /*pstr*/, UINT /*cchMax*/) const
	{
		return FALSE;
	}
	virtual UINT GetDisplayValueLength() const
	{
		return 0;
	}
	virtual BOOL GetValue(VARIANT* /*pValue*/) const
	{
		return FALSE;
	}
	virtual BOOL SetValue(const VARIANT& /*value*/)
	{
		ATLASSERT(false);
		return FALSE;
	}
	virtual BOOL SetValue(HWND /*hWnd*/)
	{
		ATLASSERT(false);
		return FALSE;
	}
};


/////////////////////////////////////////////////////////////////////////////
// Simple property (displays text)

class CPropertyItem : public CProperty
{
protected:
	CComVariant m_val;

public:
	CPropertyItem(LPCTSTR pstrName, LPARAM lParam) : CProperty(pstrName, lParam)
	{
	}
	BYTE GetKind() const
	{
		return PROPKIND_SIMPLE;
	}
	void DrawValue(PROPERTYDRAWINFO& di)
	{
		UINT cchMax = GetDisplayValueLength() + 1;
		CString text;
		int ret = GetDisplayValue(text.GetBuffer(cchMax), cchMax);
		text.ReleaseBuffer();

		if (!ret) return;
		CDCHandle dc(di.hDC);
		dc.SetBkMode(TRANSPARENT);
		dc.SetTextColor((di.state & ODS_DISABLED) != 0 ? di.clrDisabled : di.clrText);
		dc.SetBkColor(di.clrBack);
		RECT rcText = di.rcItem;
		rcText.left += PROP_TEXT_INDENT;
		dc.DrawText(text, -1, &rcText, DT_LEFT | DT_SINGLELINE | DT_EDITCONTROL | DT_NOPREFIX | DT_END_ELLIPSIS | DT_VCENTER);
	}
	BOOL GetDisplayValue(LPTSTR pstr, UINT cchMax) const
	{
		ATLASSERT(!::IsBadStringPtr(pstr, cchMax));
		// Convert VARIANT to string and use that as display string...
		CComVariant v;
		if (FAILED(v.ChangeType(VT_BSTR, &m_val))) return FALSE;
		USES_CONVERSION;
		::lstrcpyn(pstr, OLE2CT(v.bstrVal), cchMax);
		return TRUE;
	}
	UINT GetDisplayValueLength() const
	{
		// Hmm, need to convert it to display string first and
		// then take the length...
		// TODO: Call GetDisplayValue() instead...
		CComVariant v;
		if (FAILED(v.ChangeType(VT_BSTR, &m_val))) return 0;
		USES_CONVERSION;
		return v.bstrVal == NULL ? 0 : ::lstrlen(OLE2CT(v.bstrVal));
	}
	BOOL GetValue(VARIANT* pVal) const
	{
		return SUCCEEDED(CComVariant(m_val).Detach(pVal));
	}
	BOOL SetValue(const VARIANT& value)
	{
		m_val = value;
		return TRUE;
	}
};


/////////////////////////////////////////////////////////////////////////////
// Simple Value property

class CPropertyEditItem : public CPropertyItem
{
protected:
	HWND m_hwndEdit;

public:
	CPropertyEditItem(LPCTSTR pstrName, LPARAM lParam) :
	  CPropertyItem(pstrName, lParam),
		  m_hwndEdit(NULL)
	  {
	  }
	  CPropertyEditItem(LPCTSTR pstrName, CComVariant vValue, LPARAM lParam) :
	  CPropertyItem(pstrName, lParam),
		  m_hwndEdit(NULL)
	  {
		  m_val = vValue;
	  }
	  BYTE GetKind() const
	  {
		  return PROPKIND_EDIT;
	  }
	  HWND CreateInplaceControl(HWND hWnd, const RECT& rc)
	  {
		  // Get default text
		  UINT cchMax = GetDisplayValueLength() + 1;
		  CString text;
		  int ret = GetDisplayValue(text.GetBuffer(cchMax), cchMax);
		  text.ReleaseBuffer();

		  if (!ret) return NULL;
		  // Create EDIT control
		  CPropertyEditWindow* win = new CPropertyEditWindow();
		  ATLASSERT(win);
		  RECT rcWin = rc;
		  m_hwndEdit = win->Create(hWnd, rcWin, text, WS_VISIBLE | WS_CHILD | ES_LEFT | ES_AUTOHSCROLL);
		  ATLASSERT(::IsWindow(m_hwndEdit));
		  // Simple hack to validate numbers
		  switch (m_val.vt)
		  {
		  case VT_UI1:
		  case VT_UI2:
		  case VT_UI4:
			  win->ModifyStyle(0, ES_NUMBER);
			  break;
		  }
		  return m_hwndEdit;
	  }
	  BOOL SetValue(const VARIANT& value)
	  {
		  if (m_val.vt == VT_EMPTY) m_val = value;
		  return SUCCEEDED(m_val.ChangeType(m_val.vt, &value));
	  }
	  BOOL SetValue(HWND hWnd)
	  {
		  ATLASSERT(::IsWindow(hWnd));
		  int len = ::GetWindowTextLength(hWnd) + 1;
		  CString text;
		  int ret = ::GetWindowText(hWnd, text.GetBuffer(len), len);
		  text.ReleaseBuffer();

		  if (!ret)
		  {
			  // Bah, an empty string *and* an error causes the same return code!
			  if (::GetLastError() != ERROR_SUCCESS)
				  return FALSE;
		  }

		  return SetValue(CComVariant(text));
	  }
	  BOOL Activate(UINT action, LPARAM /*lParam*/)
	  {
		  switch (action)
		  {
		  case PACT_TAB:
		  case PACT_SPACE:
		  case PACT_DBLCLICK:
			  if (::IsWindow(m_hwndEdit))
			  {
				  ::SetFocus(m_hwndEdit);
				  ::SendMessage(m_hwndEdit, EM_SETSEL, 0, -1);
			  }
			  break;
		  }
		  return TRUE;
	  }
};

/////////////////////////////////////////////////////////////////////////////
// DropDown List property

class CPropertyListItem : public CPropertyItem
{
protected:
	CSimpleArray<CComBSTR> m_arrList;
	HWND m_hwndCombo;

public:
	CPropertyListItem(LPCTSTR pstrName, LPARAM lParam) :
	  CPropertyItem(pstrName, lParam),
		  m_hwndCombo(NULL)
	  {
		  m_val = -1L;
	  }
	  CPropertyListItem(LPCTSTR pstrName, LPCTSTR* ppList, int iValue, LPARAM lParam) :
	  CPropertyItem(pstrName, lParam),
		  m_hwndCombo(NULL)
	  {
		  m_val = -1L;
		  if (ppList != NULL)
		  {
			  SetList(ppList);
			  SetValue(CComVariant(iValue));
		  }
	  }
	  BYTE GetKind() const
	  {
		  return PROPKIND_LIST;
	  }
	  HWND CreateInplaceControl(HWND hWnd, const RECT& rc)
	  {
		  // Get default text
		  UINT cchMax = GetDisplayValueLength() + 1;
		  CString text;
		  int ret = GetDisplayValue(text.GetBuffer(cchMax), cchMax);
		  text.ReleaseBuffer();

		  if (!ret) return NULL;
		  // Create 'faked' DropDown control
		  CPropertyListWindow* win = new CPropertyListWindow();
		  ATLASSERT(win);
		  RECT rcWin = rc;
		  m_hwndCombo = win->Create(hWnd, rcWin, text);
		  ATLASSERT(win->IsWindow());
		  // Add to list
		  USES_CONVERSION;
		  for (int i = 0; i < m_arrList.GetSize(); ++i) win->AddItem(OLE2CT(m_arrList[i]));
		  win->SelectItem(m_val.lVal);
		  // Go...
		  return *win;
	  }
	  BOOL Activate(UINT action, LPARAM /*lParam*/)
	  {
		  switch (action)
		  {
		  case PACT_SPACE:
			  if (::IsWindow(m_hwndCombo))
			  {
				  // Fake button click...
				  ::SendMessage(m_hwndCombo, WM_COMMAND, MAKEWPARAM(0, BN_CLICKED), 0);
			  }
			  break;
		  case PACT_DBLCLICK:
			  // Simulate neat VB control effect. DblClick cycles items in list.
			  // Set value and recycle edit control
			  if (IsEnabled())
			  {
				  CComVariant v = m_val.lVal + 1L;
				  ::SendMessage(m_hWndOwner, WM_USER_PROP_CHANGEDPROPERTY, (WPARAM)(VARIANT*) &v, (LPARAM) this);
			  }
			  break;
		  }
		  return TRUE;
	  }
	  BOOL GetDisplayValue(LPTSTR pstr, UINT cchMax) const
	  {
		  ATLASSERT(m_val.vt == VT_I4);
		  ATLASSERT(!::IsBadStringPtr(pstr, cchMax));
		  *pstr = _T('\0');
		  if (m_val.lVal < 0 || m_val.lVal >= m_arrList.GetSize()) return FALSE;
		  USES_CONVERSION;
		  ::lstrcpyn(pstr, OLE2CT(m_arrList[m_val.lVal]), cchMax) ;
		  return TRUE;
	  }
	  UINT GetDisplayValueLength() const
	  {
		  ATLASSERT(m_val.vt == VT_I4);
		  if (m_val.lVal < 0 || m_val.lVal >= m_arrList.GetSize()) return 0;
		  BSTR bstr = m_arrList[m_val.lVal];
		  USES_CONVERSION;
		  return bstr == NULL ? 0 : ::lstrlen(OLE2CT(bstr));
	  };

	  BOOL SetValue(const VARIANT& value)
	  {
		  switch (value.vt)
		  {
		  case VT_BSTR:
			  {
				  m_val = 0L;
				  for (int i = 0; i < m_arrList.GetSize(); ++i)
				  {
					  if (::wcscmp(value.bstrVal, m_arrList[i]) == 0)
					  {
						  m_val = (long) i;
						  return TRUE;
					  }
				  }
				  return FALSE;
			  }
			  break;
		  default:
			  // Treat as index into list
			  if (FAILED(m_val.ChangeType(VT_I4, &value))) return FALSE;
			  if (m_val.lVal >= m_arrList.GetSize()) m_val.lVal = 0L;
			  return TRUE;
		  }
	  }
	  BOOL SetValue(HWND hWnd)
	  {
		  ATLASSERT(::IsWindow(hWnd));
		  int len = ::GetWindowTextLength(hWnd) + 1;
		  CString text;	  
		  int ret = ::GetWindowText(hWnd, text.GetBuffer(len), len);
		  text.ReleaseBuffer();

		  if (!ret)
		  {
			  if (::GetLastError() != ERROR_SUCCESS)
			  {
				  return FALSE;
			  }
		  }

		  return SetValue(CComVariant(text));
	  }
	  void SetList(LPCTSTR* ppList)
	  {
		  ATLASSERT(ppList);
		  m_arrList.RemoveAll();
		  while (*ppList != NULL)
		  {
			  CComBSTR bstr(*ppList);
			  m_arrList.Add(bstr);
			  ++ppList;
		  }
		  if (m_val.lVal < 0L) m_val = 0L;
		  if (m_val.lVal >= (LONG) m_arrList.GetSize()) m_val = 0L;
	  }
	  void AddListItem(LPCTSTR pstrText)
	  {
		  ATLASSERT(!::IsBadStringPtr(pstrText, -1));
		  CComBSTR bstr(pstrText);
		  m_arrList.Add(bstr);
		  if (m_val.lVal < 0L) m_val = 0L;
	  }
};


/////////////////////////////////////////////////////////////////////////////
// Boolean property

class CPropertyBooleanItem : public CPropertyListItem
{
public:
	CPropertyBooleanItem(LPCTSTR pstrName, LPARAM lParam) : CPropertyListItem(pstrName, lParam)
	{
		_InitBooleanList();
	}
	CPropertyBooleanItem(LPCTSTR pstrName, bool bValue, LPARAM lParam) : CPropertyListItem(pstrName, lParam)
	{
		_InitBooleanList();
		SetValue(CComVariant(bValue));
	}

	BOOL SetValue(const VARIANT& value)
	{
		// Convert to list index...
		if (value.vt == VT_BOOL) return CPropertyListItem::SetValue(CComVariant(value.boolVal ? 1L : 0L));
		return CPropertyListItem::SetValue(value);
	}

	VOID _InitBooleanList()
	{
		AddListItem(_T("False"));
		AddListItem(_T("True"));
	}
};


/////////////////////////////////////////////////////////////////////////////
//
// CProperty creators
//

inline HPROPERTY PropCreateVariant(LPCTSTR pstrName, const VARIANT& vValue, LPARAM lParam = 0)
{
	return new CPropertyEditItem(pstrName, vValue, lParam);
}

inline HPROPERTY PropCreateSimple(LPCTSTR pstrName, LPCTSTR pstrValue, LPARAM lParam = 0)
{
	return new CPropertyEditItem(pstrName, CComVariant(pstrValue), lParam);
}

inline HPROPERTY PropCreateSimple(LPCTSTR pstrName, int iValue, LPARAM lParam = 0)
{
	return new CPropertyEditItem(pstrName, CComVariant(iValue), lParam);
}

inline HPROPERTY PropCreateSimple(LPCTSTR pstrName, bool bValue, LPARAM lParam = 0)
{
	return new CPropertyBooleanItem(pstrName, bValue, lParam);
}

inline HPROPERTY PropCreateList(LPCTSTR pstrName, LPCTSTR* ppList, int iValue = 0, LPARAM lParam = 0)
{
	return new CPropertyListItem(pstrName, ppList, iValue, lParam);
}

#endif // __PROPERTYITEMIMPL__H
