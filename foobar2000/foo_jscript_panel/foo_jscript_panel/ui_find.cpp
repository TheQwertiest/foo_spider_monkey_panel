#include "stdafx.h"
#include "ui_find.h"
#include "ui_conf.h"


LRESULT CDialogFind::OnFindUp(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	if (m_text.is_empty())
		return 0;

	CDialogConf::FindPrevious(m_hWnd, m_hedit, m_flags, m_text.get_ptr());
	return 0;
}

LRESULT CDialogFind::OnFindDown(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	if (m_text.is_empty())
		return 0;

	CDialogConf::FindNext(m_hWnd, m_hedit, m_flags, m_text.get_ptr());
	return 0;
}

LRESULT CDialogFind::OnEditFindWhatEnChange(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	uGetWindowText(GetDlgItem(IDC_EDIT_FINDWHAT), m_text);
	return 0;
}

LRESULT CDialogFind::OnFlagCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	bool check = uButton_GetCheck(m_hWnd, wID);
	int flag = 0;

	switch (wID)
	{
	case IDC_CHECK_MATCHCASE:
		flag = SCFIND_MATCHCASE;
		break;

	case IDC_CHECK_WHOLEWORD:
		flag = SCFIND_WHOLEWORD;
		break;

	case IDC_CHECK_WORDSTART:
		flag = SCFIND_WORDSTART;
		break;

	case IDC_CHECK_REGEXP:
		flag = SCFIND_REGEXP;
		break;
	}

	if (check)
		m_flags |= flag;
	else
		m_flags &= ~flag;

	return 0;
}

LRESULT CDialogFind::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	ShowWindow(SW_HIDE);
	return 0;
}

void CDialogFind::OnFinalMessage(HWND hWnd)
{
	modeless_dialog_manager::g_remove(m_hWnd);
	delete this;
}

LRESULT CDialogFind::OnInitDialog(HWND hwndFocus, LPARAM lParam)
{
	modeless_dialog_manager::g_add(m_hWnd);
	DlgResize_Init();
	m_find.SubclassWindow(GetDlgItem(IDC_EDIT_FINDWHAT), m_hWnd);
	return TRUE; // set focus to default control
}
