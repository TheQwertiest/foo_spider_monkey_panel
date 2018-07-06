#pragma once

#include "resource.h"

class CDialogReplace : public CDialogImpl<CDialogReplace>, public CDialogResize<CDialogReplace>
{
public:
	CDialogReplace(HWND p_hedit) : m_hedit(p_hedit), m_flags(0), m_havefound(false)
	{
	}

	BEGIN_DLGRESIZE_MAP(CDialogReplace)
		DLGRESIZE_CONTROL(IDC_EDIT_FINDWHAT, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_EDIT_REPLACE, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_FINDNEXT, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_REPLACE, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_REPLACEALL, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X)
	END_DLGRESIZE_MAP()

	BEGIN_MSG_MAP(CDialogReplace)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_ID_HANDLER_EX(IDC_FINDNEXT, OnFindNext)
		COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
		COMMAND_HANDLER_EX(IDC_EDIT_FINDWHAT, EN_CHANGE, OnEditFindWhatEnChange)
		COMMAND_HANDLER_EX(IDC_EDIT_REPLACE, EN_CHANGE, OnEditReplaceEnChange)
		COMMAND_RANGE_HANDLER_EX(IDC_CHECK_MATCHCASE, IDC_CHECK_REGEXP, OnFlagCommand)
		COMMAND_ID_HANDLER_EX(IDC_REPLACE, OnReplace)
		COMMAND_ID_HANDLER_EX(IDC_REPLACEALL, OnReplaceall)
		CHAIN_MSG_MAP(CDialogResize<CDialogReplace>)
	END_MSG_MAP()

	enum
	{
		IDD = IDD_DIALOG_REPLACE
	};

	CHARRANGE GetSelection();
	LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnEditFindWhatEnChange(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnEditReplaceEnChange(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnFindNext(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnFlagCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnInitDialog(HWND hwndFocus, LPARAM lParam);
	LRESULT OnReplace(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnReplaceall(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	void OnFinalMessage(HWND hWnd);

private:
	class CEditWithReturn : public CWindowImpl<CEditWithReturn, CEdit>
	{
	public:
		typedef CWindowImpl<CEditWithReturn, CEdit> parent;

		BEGIN_MSG_MAP(CEditWithReturn)
			MESSAGE_HANDLER(WM_CHAR, OnChar)
			MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
		END_MSG_MAP()

		BOOL SubclassWindow(HWND hWnd, HWND hParent)
		{
			m_parent = hParent;
			return parent::SubclassWindow(hWnd);
		}

		LRESULT OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
		{
			// Disable anonying sound
			switch (wParam)
			{
			case '\n':
			case '\r':
			case '\t':
			case '\x1b':
				return 0;
			}

			return DefWindowProc(uMsg, wParam, lParam);
		}

		LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
		{
			switch (wParam)
			{
			case VK_RETURN:
				::PostMessage(m_parent, WM_COMMAND, MAKEWPARAM(IDC_REPLACE, BN_CLICKED), (LPARAM)m_hWnd);
				return FALSE;

			case VK_ESCAPE:
				::PostMessage(m_parent, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED), (LPARAM)m_hWnd);
				return FALSE;

			case VK_TAB:
				::PostMessage(m_parent, WM_NEXTDLGCTL, 0, 0);
				return FALSE;
			}

			return DefWindowProc(uMsg, wParam, lParam);
		}

	private:
		HWND m_parent;
	};

	bool m_havefound;
	CEditWithReturn m_find, m_replace;
	HWND m_hedit;
	int m_flags;
	pfc::string8 m_reptext;
	pfc::string8 m_text;
};
