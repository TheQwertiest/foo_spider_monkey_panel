#include "stdafx.h"
#include "ui_name_value_edit.h"


LRESULT CNameValueEdit::OnInitDialog(HWND hwndFocus, LPARAM lParam)
{
	uSendDlgItemMessageText(m_hWnd, IDC_EDIT_NAME, WM_SETTEXT, 0, m_name);
	uSendDlgItemMessageText(m_hWnd, IDC_EDIT_VALUE, WM_SETTEXT, 0, m_value);

	// Select all
	SendDlgItemMessage(IDC_EDIT_VALUE, EM_SETSEL, 0, -1);
	::SetFocus(GetDlgItem(IDC_EDIT_VALUE));

	return FALSE; 
}

LRESULT CNameValueEdit::OnCommand(UINT codeNotify, int id, HWND hwndCtl)
{
	if (id == IDOK || id == IDCANCEL)
	{
		if (id == IDOK)
			uGetDlgItemText(m_hWnd, IDC_EDIT_VALUE, m_value);

		EndDialog(id);
	}

	return 0;
}
