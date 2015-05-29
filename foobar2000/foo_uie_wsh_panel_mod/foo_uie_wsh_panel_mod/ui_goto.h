#pragma once

#include "resource.h"


class CDialogGoto : public CDialogImpl<CDialogGoto>
{
private:
	HWND m_hedit;

public:
	enum { IDD = IDD_DIALOG_GOTO };

	BEGIN_MSG_MAP(CDialogGoto)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_RANGE_HANDLER_EX(IDOK, IDCANCEL, OnCloseCmd)
	END_MSG_MAP()

public:
	CDialogGoto(HWND p_hedit) : m_hedit(p_hedit)
	{
	}

public:
	LRESULT OnInitDialog(HWND hwndFocus, LPARAM lParam);
	LRESULT OnCloseCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl);
};
