#pragma once

#include "config.h"
#include "resource.h"
#include "PropertyList.h"


// Forward declarations
class wsh_panel_window;


class CDialogProperty
	: public CDialogImpl<CDialogProperty>
	, public CDialogResize<CDialogProperty>
{
private:
	wsh_panel_window * m_parent;
	CPropertyListCtrl m_properties;
	prop_kv_config::t_map m_dup_prop_map;

public:
	CDialogProperty(wsh_panel_window * p_parent) : m_parent(p_parent)
	{
	}

	virtual ~CDialogProperty()
	{

	}

	void LoadProperties(bool reload = true);
	void Apply();

public:
	enum { IDD = IDD_DIALOG_PROPERTIES };

	BEGIN_MSG_MAP(CDialogProperty)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_RANGE_HANDLER_EX(IDOK, IDCANCEL, OnCloseCmd)
		COMMAND_ID_HANDLER_EX(IDAPPLY, OnCloseCmd)
		COMMAND_HANDLER_EX(IDC_CLEARALL, BN_CLICKED, OnClearallBnClicked)
		COMMAND_HANDLER_EX(IDC_DEL, BN_CLICKED, OnDelBnClicked)
		COMMAND_HANDLER_EX(IDC_IMPORT, BN_CLICKED, OnImportBnClicked)
		COMMAND_HANDLER_EX(IDC_EXPORT, BN_CLICKED, OnExportBnClicked)
		NOTIFY_CODE_HANDLER_EX(PIN_ITEMCHANGED, OnPinItemChanged)
		CHAIN_MSG_MAP(CDialogResize<CDialogProperty>)
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(CDialogProperty)
		DLGRESIZE_CONTROL(IDC_LIST_PROPERTIES, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_DEL, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_CLEARALL, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_IMPORT, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_EXPORT, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDAPPLY, DLSZ_MOVE_X | DLSZ_MOVE_Y)
	END_DLGRESIZE_MAP()

public:
	LRESULT OnInitDialog(HWND hwndFocus, LPARAM lParam);
	LRESULT OnCloseCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnPinItemChanged(LPNMHDR pnmh);
	LRESULT OnClearallBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnDelBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnImportBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnExportBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl);
};
