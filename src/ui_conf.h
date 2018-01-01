#pragma once

#include "editorctrl.h"
#include "resource.h"

// Forward declarations
class js_panel_window;
class CDialogFind;
class CDialogReplace;

class CDialogConf : public CDialogImpl<CDialogConf>, public CDialogResize<CDialogConf>
{
public:
	CDialogConf(js_panel_window* p_parent) : m_parent(p_parent), m_dlgfind(NULL), m_dlgreplace(NULL), m_lastSearchText(""), m_lastFlags(0)
	{
	}

	virtual ~CDialogConf()
	{
		m_hWnd = NULL;
	}

	BEGIN_DLGRESIZE_MAP(CDialogConf)
		DLGRESIZE_CONTROL(IDC_STATIC_GUID, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_EDIT, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_TOOLS, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_STATIC_ENGINE, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_COMBO_ENGINE, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_STATIC_EDGE, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_COMBO_EDGE, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_CHECK_PSEUDO_TRANSPARENT, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_CHECK_GRABFOCUS, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDAPPLY, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
	END_DLGRESIZE_MAP()

	BEGIN_MSG_MAP(CDialogConf)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_NOTIFY(OnNotify)
		MESSAGE_HANDLER(UWM_KEYDOWN, OnUwmKeyDown)
		MESSAGE_HANDLER(UWM_FINDTEXTCHANGED, OnUwmFindTextChanged)
		COMMAND_RANGE_HANDLER_EX(IDOK, IDCANCEL, OnCloseCmd)
		COMMAND_ID_HANDLER_EX(IDAPPLY, OnCloseCmd)
		COMMAND_ID_HANDLER_EX(IDC_TOOLS, OnTools)
		CHAIN_MSG_MAP(CDialogResize<CDialogConf>)
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

	enum
	{
		IDD = IDD_DIALOG_CONFIG
	};

	LRESULT OnCloseCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnInitDialog(HWND hwndFocus, LPARAM lParam);
	LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
	LRESULT OnTools(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnUwmFindTextChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnUwmKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	bool MatchShortcuts(unsigned vk);
	static bool FindNext(HWND hWnd, HWND hWndEdit, unsigned flags, const char* which);
	static bool FindPrevious(HWND hWnd, HWND hWndEdit, unsigned flags, const char* which);
	static bool FindResult(HWND hWnd, HWND hWndEdit, int pos, const char* which);
	void Apply();
	void OnExport();
	void OnImport();
	void OnResetCurrent();
	void OnResetDefault();
	void OpenFindDialog();

private:
	CScriptEditorCtrl m_editorctrl;
	CDialogFind* m_dlgfind;
	CDialogReplace* m_dlgreplace;
	js_panel_window* m_parent;
	pfc::string8 m_caption;
	unsigned int m_lastFlags;
	pfc::string8 m_lastSearchText;
};
