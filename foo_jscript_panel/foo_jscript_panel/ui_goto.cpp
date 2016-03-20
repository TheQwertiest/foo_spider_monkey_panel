#include "stdafx.h"
#include "ui_goto.h"


LRESULT CDialogGoto::OnInitDialog(HWND hwndFocus, LPARAM lParam)
{
	int cur_pos = SendMessage(m_hedit, SCI_GETCURRENTPOS, 0, 0);
	int cur_line = SendMessage(m_hedit, SCI_LINEFROMPOSITION, cur_pos, 0);

	uSetWindowText(GetDlgItem(IDC_EDIT_LINENUMBER), pfc::format_int(cur_line + 1).get_ptr());

	return TRUE; // set focus to default control
}

LRESULT CDialogGoto::OnCloseCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	if (wID == IDOK)
	{
		pfc::string8 text;
		unsigned i = 0;
		
		uGetWindowText(GetDlgItem(IDC_EDIT_LINENUMBER), text);
		i = pfc::atoui_ex(text.get_ptr(), text.length()) - 1;
		SendMessage(m_hedit, SCI_GOTOLINE, i, 0);
	}

	EndDialog(wID);
	return 0;
}
