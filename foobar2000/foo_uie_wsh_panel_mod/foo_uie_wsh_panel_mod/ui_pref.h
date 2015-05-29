#pragma once

#include "resource.h"

class CDialogPref : public CDialogImpl<CDialogPref>
	, public CWinDataExchange<CDialogPref>
	, public preferences_page_instance
{
private:
	CListViewCtrl m_props;
	preferences_page_callback::ptr m_callback;

public:
	CDialogPref(preferences_page_callback::ptr callback) : m_callback(callback) {}

	void LoadProps(bool reset = false);
	void uGetItemText(int nItem, int nSubItem, pfc::string_base & out);

	void OnChanged();
	bool HasChanged();

	// preferences_page_instance
	HWND get_wnd();
	t_uint32 get_state();
	void apply();
	void reset();

public:
	enum { IDD = IDD_DIALOG_PREFERENCE };

	BEGIN_MSG_MAP(CDialogPref)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_BUTTON_EXPORT, BN_CLICKED, OnButtonExportBnClicked)
		COMMAND_HANDLER_EX(IDC_BUTTON_IMPORT, BN_CLICKED, OnButtonImportBnClicked)
		COMMAND_HANDLER_EX(IDC_CHECK_SAFE_MODE, BN_CLICKED, OnEditChange)
		NOTIFY_HANDLER_EX(IDC_LIST_EDITOR_PROP, NM_DBLCLK, OnPropNMDblClk)
	END_MSG_MAP()

	BEGIN_DDX_MAP(CDialogPref)
		DDX_CONTROL_HANDLE(IDC_LIST_EDITOR_PROP, m_props)
	END_DDX_MAP()

	BOOL OnInitDialog(HWND hwndFocus, LPARAM lParam);
	LRESULT OnPropNMDblClk(LPNMHDR pnmh);
	void OnButtonExportBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	void OnButtonImportBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	void OnEditChange(WORD, WORD, HWND);
};

class wsh_preferences_page_impl : public preferences_page_v3
{
public:
	preferences_page_instance::ptr instantiate(HWND parent, preferences_page_callback::ptr callback)
	{
		service_impl_t<CDialogPref> * p = new service_impl_t<CDialogPref>(callback);
		p->Create(parent);
		return p;
	}

	const char * get_name()
	{
		return WSPM_NAME;
	}

	GUID get_guid()
	{
		return g_ui_pref_window_guid;
	}
	
	GUID get_parent_guid()
	{
		return preferences_page::guid_tools;
	}

	bool get_help_url(pfc::string_base & p_out)
	{
		p_out = "http://code.google.com/p/foo-wsh-panel-mod/wiki/EditorProperties";
		return true;
	}
};
