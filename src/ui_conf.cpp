#include "stdafx.h"
#include "ui_conf.h"
#include "js_panel_window.h"
#include "ui_goto.h"
#include "ui_find.h"
#include "ui_replace.h"
#include "helpers.h"

LRESULT CDialogConf::OnInitDialog(HWND hwndFocus, LPARAM lParam)
{
	// Get caption text
	uGetWindowText(m_hWnd, m_caption);

	// Init resize
	DlgResize_Init();

	// Apply window placement
	if (m_parent->get_windowplacement().length == 0)
	{
		m_parent->get_windowplacement().length = sizeof(WINDOWPLACEMENT);

		if (!GetWindowPlacement(&m_parent->get_windowplacement()))
		{
			memset(&m_parent->get_windowplacement(), 0, sizeof(WINDOWPLACEMENT));
		}
	}
	else
	{
		SetWindowPlacement(&m_parent->get_windowplacement());
	}

	// GUID Text
	pfc::string8 guid_text = "GUID: ";
	guid_text += pfc::print_guid(m_parent->get_config_guid());
	uSetWindowText(GetDlgItem(IDC_STATIC_GUID), guid_text);

	// Edit Control
	m_editorctrl.SubclassWindow(GetDlgItem(IDC_EDIT));
	m_editorctrl.SetJScript();
	m_editorctrl.ReadAPI();
	m_editorctrl.SetContent(m_parent->get_script_code(), true);
	m_editorctrl.SetSavePoint();

	// Script Engine
	HWND combo_engine = GetDlgItem(IDC_COMBO_ENGINE);
	ComboBox_AddString(combo_engine, _T("Chakra"));
	ComboBox_AddString(combo_engine, _T("JScript"));
	if (IsWindowsVistaOrGreater())
	{
		uComboBox_SelectString(combo_engine, m_parent->get_script_engine());
	}
	else
	{
		GetDlgItem(IDC_COMBO_ENGINE).EnableWindow(false);
	}

	if (m_parent->GetInstanceType() == HostComm::KInstanceTypeCUI)
	{
		// Edge Style
		HWND combo_edge = GetDlgItem(IDC_COMBO_EDGE);
		ComboBox_AddString(combo_edge, _T("None"));
		ComboBox_AddString(combo_edge, _T("Sunken"));
		ComboBox_AddString(combo_edge, _T("Grey"));
		ComboBox_SetCurSel(combo_edge, m_parent->get_edge_style());

		// Pseudo Transparent
		uButton_SetCheck(m_hWnd, IDC_CHECK_PSEUDO_TRANSPARENT, m_parent->get_pseudo_transparent());
	}
	else
	{
		// Disable these items in Default UI

		// Edge Style
		HWND combo_edge = GetDlgItem(IDC_COMBO_EDGE);
		GetDlgItem(IDC_COMBO_EDGE).EnableWindow(false);

		// Pseudo
		uButton_SetCheck(m_hWnd, IDC_CHECK_PSEUDO_TRANSPARENT, false);
		GetDlgItem(IDC_CHECK_PSEUDO_TRANSPARENT).EnableWindow(false);
	}

	// Grab Focus
	uButton_SetCheck(m_hWnd, IDC_CHECK_GRABFOCUS, m_parent->get_grab_focus());

	return TRUE; // set focus to default control
}

LRESULT CDialogConf::OnCloseCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	switch (wID)
	{
	case IDOK:
		Apply();
		EndDialog(IDOK);
		break;

	case IDAPPLY:
		Apply();
		break;

	case IDCANCEL:
		if (m_editorctrl.GetModify())
		{
			// Prompt?
			int ret = uMessageBox(m_hWnd, "Do you want to apply your changes?", m_caption, MB_ICONWARNING | MB_SETFOREGROUND | MB_YESNOCANCEL);

			switch (ret)
			{
			case IDYES:
				Apply();
				EndDialog(IDOK);
				break;

			case IDCANCEL:
				return 0;
			}
		}

		EndDialog(IDCANCEL);
	}

	return 0;
}

void CDialogConf::OnResetDefault()
{
	HWND combo = GetDlgItem(IDC_COMBO_ENGINE);
	uComboBox_SelectString(combo, "Chakra");
	pfc::string8 code;
	js_panel_vars::get_default_script_code(code);
	m_editorctrl.SetContent(code);
}

void CDialogConf::OnResetCurrent()
{
	HWND combo = GetDlgItem(IDC_COMBO_ENGINE);
	uComboBox_SelectString(combo, m_parent->get_script_engine());
	m_editorctrl.SetContent(m_parent->get_script_code());
}

void CDialogConf::OnImport()
{
	pfc::string8 filename;

	if (uGetOpenFileName(m_hWnd, "Text files|*.txt|JScript files|*.js|All files|*.*", 0, "txt", "Import from", NULL, filename, FALSE))
	{
		// Open file
		pfc::string8_fast text;

		helpers::read_file(filename, text);
		m_editorctrl.SetContent(text);
	}
}

void CDialogConf::OnExport()
{
	pfc::string8 filename;

	if (uGetOpenFileName(m_hWnd, "Text files|*.txt|All files|*.*", 0, "txt", "Save as", NULL, filename, TRUE))
	{
		int len = m_editorctrl.GetTextLength();
		pfc::string8_fast text;

		m_editorctrl.GetText(text.lock_buffer(len), len + 1);
		text.unlock_buffer();

		helpers::write_file(filename, text);
	}
}

LRESULT CDialogConf::OnTools(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	enum
	{
		kImport = 1,
		kExport,
		kResetDefault,
		kResetCurrent,
	};

	HMENU menu = CreatePopupMenu();
	int ret = 0;
	int flags = TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD;
	RECT rc = { 0 };

	AppendMenu(menu, MF_STRING, kImport, _T("&Import"));
	AppendMenu(menu, MF_STRING, kExport, _T("E&xport"));
	AppendMenu(menu, MF_SEPARATOR, 0, 0);
	AppendMenu(menu, MF_STRING, kResetDefault, _T("Reset &Default"));
	AppendMenu(menu, MF_STRING, kResetCurrent, _T("Reset &Current"));

	::GetWindowRect(::GetDlgItem(m_hWnd, IDC_TOOLS), &rc);

	ret = TrackPopupMenu(menu, flags, rc.left, rc.bottom, 0, m_hWnd, 0);

	switch (ret)
	{
	case kImport:
		OnImport();
		break;

	case kExport:
		OnExport();
		break;

	case kResetDefault:
		OnResetDefault();
		break;

	case kResetCurrent:
		OnResetCurrent();
		break;
	}

	DestroyMenu(menu);
	return 0;
}

void CDialogConf::Apply()
{
	pfc::string8 name;
	pfc::array_t<char> code;
	int len = 0;

	// Get engine name
	uGetWindowText(GetDlgItem(IDC_COMBO_ENGINE), name);
	// Get script text
	len = m_editorctrl.GetTextLength();
	code.set_size(len + 1);
	m_editorctrl.GetText(code.get_ptr(), len + 1);

	m_parent->get_edge_style() = static_cast<t_edge_style>(ComboBox_GetCurSel(GetDlgItem(IDC_COMBO_EDGE)));
	m_parent->get_disabled_before() = false;
	m_parent->get_grab_focus() = uButton_GetCheck(m_hWnd, IDC_CHECK_GRABFOCUS);
	m_parent->get_pseudo_transparent() = uButton_GetCheck(m_hWnd, IDC_CHECK_PSEUDO_TRANSPARENT);
	m_parent->update_script(name, code.get_ptr());

	// Wndow position
	GetWindowPlacement(&m_parent->get_windowplacement());

	// Save point
	m_editorctrl.SetSavePoint();
}

LRESULT CDialogConf::OnNotify(int idCtrl, LPNMHDR pnmh)
{
	SCNotification* notification = (SCNotification *)pnmh;

	switch (pnmh->code)
	{
		// dirty
	case SCN_SAVEPOINTLEFT:
	{
		pfc::string8 caption = m_caption;

		caption += " *";
		uSetWindowText(m_hWnd, caption);
	}
	break;

	// not dirty
	case SCN_SAVEPOINTREACHED:
		uSetWindowText(m_hWnd, m_caption);
		break;
	}

	SetMsgHandled(FALSE);
	return 0;
}

bool CDialogConf::MatchShortcuts(unsigned vk)
{
	int modifiers =
		(IsKeyPressed(VK_SHIFT) ? SCMOD_SHIFT : 0) |
		(IsKeyPressed(VK_CONTROL) ? SCMOD_CTRL : 0) |
		(IsKeyPressed(VK_MENU) ? SCMOD_ALT : 0);

	// Hotkeys
	if (modifiers == SCMOD_CTRL)
	{
		switch (vk)
		{
		case 'F':
			OpenFindDialog();
			return true;

		case 'H':
		{
			if (!m_dlgreplace)
			{
				m_dlgreplace = new CDialogReplace(GetDlgItem(IDC_EDIT));

				if (!m_dlgreplace || !m_dlgreplace->Create(m_hWnd))
				{
					break;
				}
			}

			m_dlgreplace->ShowWindow(SW_SHOW);
			m_dlgreplace->SetFocus();
		}
		return true;

		case 'G':
		{
			modal_dialog_scope scope(m_hWnd);
			CDialogGoto dlg(GetDlgItem(IDC_EDIT));
			dlg.DoModal(m_hWnd);
		}
		return true;

		case 'S':
			Apply();
			return true;
		}
	}
	else if (modifiers == 0)
	{
		if (vk == VK_F3)
		{
			// Find next one
			if (!m_lastSearchText.is_empty())
			{
				FindNext(m_hWnd, m_editorctrl.m_hWnd, m_lastFlags, m_lastSearchText);
			}
			else
			{
				OpenFindDialog();
			}
		}
	}
	else if (modifiers == SCMOD_SHIFT)
	{
		if (vk == VK_F3)
		{
			// Find previous one
			if (!m_lastSearchText.is_empty())
			{
				FindPrevious(m_hWnd, m_editorctrl.m_hWnd, m_lastFlags, m_lastSearchText);
			}
			else
			{
				OpenFindDialog();
			}
		}
	}

	return false;
}

LRESULT CDialogConf::OnUwmKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return MatchShortcuts(wParam);
}

LRESULT CDialogConf::OnUwmFindTextChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	m_lastFlags = wParam;
	m_lastSearchText = reinterpret_cast<const char*>(lParam);
	return 0;
}

bool CDialogConf::FindNext(HWND hWnd, HWND hWndEdit, unsigned flags, const char* which)
{
	::SendMessage(::GetAncestor(hWndEdit, GA_PARENT), UWM_FINDTEXTCHANGED, flags, reinterpret_cast<LPARAM>(which));

	SendMessage(hWndEdit, SCI_CHARRIGHT, 0, 0);
	SendMessage(hWndEdit, SCI_SEARCHANCHOR, 0, 0);
	int pos = ::SendMessage(hWndEdit, SCI_SEARCHNEXT, flags, reinterpret_cast<LPARAM>(which));
	return FindResult(hWnd, hWndEdit, pos, which);
}

bool CDialogConf::FindPrevious(HWND hWnd, HWND hWndEdit, unsigned flags, const char* which)
{
	::SendMessage(::GetAncestor(hWndEdit, GA_PARENT), UWM_FINDTEXTCHANGED, flags, reinterpret_cast<LPARAM>(which));

	SendMessage(hWndEdit, SCI_SEARCHANCHOR, 0, 0);
	int pos = ::SendMessage(hWndEdit, SCI_SEARCHPREV, flags, reinterpret_cast<LPARAM>(which));
	return FindResult(hWnd, hWndEdit, pos, which);
}

bool CDialogConf::FindResult(HWND hWnd, HWND hWndEdit, int pos, const char* which)
{
	if (pos != -1)
	{
		// Scroll to view
		::SendMessage(hWndEdit, SCI_SCROLLCARET, 0, 0);
		return true;
	}

	pfc::string8 buff = "Cannot find \"";
	buff += which;
	buff += "\"";
	uMessageBox(hWnd, buff.get_ptr(), JSP_NAME, MB_ICONINFORMATION | MB_SETFOREGROUND);
	return false;
}

void CDialogConf::OpenFindDialog()
{
	if (!m_dlgfind)
	{
		// Create it on request.
		m_dlgfind = new CDialogFind(GetDlgItem(IDC_EDIT));
		m_dlgfind->Create(m_hWnd);
	}

	m_dlgfind->ShowWindow(SW_SHOW);
	m_dlgfind->SetFocus();
}
