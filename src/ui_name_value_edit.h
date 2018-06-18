#pragma once

#include "resource.h"

class CNameValueEdit : public CDialogImpl<CNameValueEdit>
{
public:
	CNameValueEdit(const char* p_name, const char* p_value) : m_name(p_name), m_value(p_value)
	{
	}

	BEGIN_MSG_MAP(CNameValueEdit)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_COMMAND(OnCommand)
	END_MSG_MAP()

	enum
	{
		IDD = IDD_DIALOG_NAME_VALUE_EDIT
	};

	LRESULT OnInitDialog(HWND hwndFocus, LPARAM lParam);
	LRESULT OnCommand(UINT codeNotify, int id, HWND hwndCtl);
	void GetValue(pfc::string_base& p_value);

private:
	pfc::string_simple m_name;
	pfc::string8 m_value;
};
