#pragma once

#include "editorctrl.h"
#include "resource.h"

// Forward declarations
class wsh_panel_window;
class CDialogFind;
class CDialogReplace;

//-----
class CDialogConf 
	: public CDialogImpl<CDialogConf>
	, public CDialogResize<CDialogConf>
{
private:
	CScriptEditorCtrl m_editorctrl;
	CDialogFind * m_dlgfind;
	CDialogReplace * m_dlgreplace;
	wsh_panel_window * m_parent;
	pfc::string8 m_caption;
	unsigned int m_lastFlags;
	pfc::string8 m_lastSearchText;

public:
	CDialogConf(wsh_panel_window * p_parent) 
		: m_parent(p_parent)
		, m_dlgfind(NULL)
		, m_dlgreplace(NULL)
		, m_lastSearchText("")
		, m_lastFlags(0)
	{
		//pfc::dynamic_assert(m_parent != NULL, "CDialogConf: m_parent invalid.");
	}

	virtual ~CDialogConf()
	{
		m_hWnd = NULL;
	}

	bool MatchShortcuts(unsigned vk);
	void OpenFindDialog();
	void Apply();
	void OnResetDefault();
	void OnResetCurrent();
	void OnImport();
	void OnExport();

public:
	enum { IDD = IDD_DIALOG_CONFIG };

	BEGIN_MSG_MAP(CDialogConf)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_NOTIFY(OnNotify)
		MESSAGE_HANDLER(UWM_KEYDOWN, OnUwmKeyDown)
		MESSAGE_HANDLER(UWM_FINDTEXTCHANGED, OnUwmFindTextChanged)
		COMMAND_RANGE_HANDLER_EX(IDOK, IDCANCEL, OnCloseCmd)
		COMMAND_ID_HANDLER_EX(IDAPPLY, OnCloseCmd)
		COMMAND_HANDLER_EX(IDC_SCRIPT_ENGINE, CBN_SELENDOK, OnScriptEngineCbnSelEndOk)
		COMMAND_ID_HANDLER_EX(IDC_TOOLS, OnTools)
		CHAIN_MSG_MAP(CDialogResize<CDialogConf>)
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(CDialogConf)
		DLGRESIZE_CONTROL(IDC_CHECK_PSEUDO_TRANSPARENT, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_CHECK_GRABFOCUS, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_CHECK_DELAY_LOAD, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_TOOLS, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_EDIT, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)  
		DLGRESIZE_CONTROL(IDAPPLY, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_STATIC_GUID, DLSZ_SIZE_X)
	END_DLGRESIZE_MAP()

public:
	LRESULT OnInitDialog(HWND hwndFocus, LPARAM lParam);
	LRESULT OnCloseCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnScriptEngineCbnSelEndOk(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnTools(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
	LRESULT OnNCDestroy();
	LRESULT OnUwmKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnUwmFindTextChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	static bool FindNext(HWND hWnd, HWND hWndEdit, unsigned flags, const char *which);
	static bool FindPrevious(HWND hWnd, HWND hWndEdit, unsigned flags, const char *which);
	static bool FindResult(HWND hWnd, HWND hWndEdit, int pos, const char *which);
};
