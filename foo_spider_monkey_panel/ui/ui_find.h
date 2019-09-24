#pragma once

#include "resource.h"

namespace smp::ui
{

class CDialogFind 
	: public CDialogImpl<CDialogFind>
    , public CDialogResize<CDialogFind>
{
public:
    CDialogFind( HWND p_hedit );

    // TODO: fix broken resizing and add centering

    BEGIN_DLGRESIZE_MAP( CDialogFind )
		DLGRESIZE_CONTROL( IDC_EDIT_FINDWHAT, DLSZ_SIZE_X )
		DLGRESIZE_CONTROL( IDC_FINDUP, DLSZ_MOVE_X )
		DLGRESIZE_CONTROL( IDC_FINDDOWN, DLSZ_MOVE_X )
		DLGRESIZE_CONTROL( IDCANCEL, DLSZ_MOVE_X )
    END_DLGRESIZE_MAP()

    BEGIN_MSG_MAP( CDialogFind )
		MSG_WM_INITDIALOG( OnInitDialog )
		COMMAND_ID_HANDLER_EX( IDC_FINDUP, OnFindUp )
		COMMAND_ID_HANDLER_EX( IDC_FINDDOWN, OnFindDown )
		COMMAND_HANDLER_EX( IDC_EDIT_FINDWHAT, EN_CHANGE, OnEditFindWhatEnChange )
		COMMAND_RANGE_HANDLER_EX( IDC_CHECK_MATCHCASE, IDC_CHECK_REGEXP, OnFlagCommand )
		COMMAND_ID_HANDLER_EX( IDCANCEL, OnCancel )
		CHAIN_MSG_MAP( CDialogResize<CDialogFind> )
    END_MSG_MAP()

    enum
    {
        IDD = IDD_DIALOG_FIND
    };

    LRESULT OnCancel( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnEditFindWhatEnChange( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnFindUp( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnFindDown( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnFlagCommand( WORD wNotifyCode, WORD wID, HWND hWndCtl );
    LRESULT OnInitDialog( HWND hwndFocus, LPARAM lParam );
	
	// CDialogImpl
    void OnFinalMessage( HWND hWnd ) override;

private:
    class CEditWithReturn : public CWindowImpl<CEditWithReturn, CEdit>
    {
    public:
        typedef CWindowImpl<CEditWithReturn, CEdit> parent;

        BOOL SubclassWindow( HWND hWnd, HWND hParent )
        {
            m_parent = hParent;
            return parent::SubclassWindow( hWnd );
        }

        BEGIN_MSG_MAP( CEditWithReturn )
			MESSAGE_HANDLER( WM_CHAR, OnChar )
			MESSAGE_HANDLER( WM_KEYDOWN, OnKeyDown )
        END_MSG_MAP()

        LRESULT OnChar( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
        {
            // Disable annoying sound
            switch ( wParam )
            {
            case '\n':
            case '\r':
            case '\t':
            case '\x1b':
                return 0;
            }

            return DefWindowProc( uMsg, wParam, lParam );
        }

        LRESULT OnKeyDown( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
        {
            switch ( wParam )
            {
            case VK_RETURN:
                ::PostMessage( m_parent, WM_COMMAND, MAKEWPARAM( IDC_FINDDOWN, BN_CLICKED ), (LPARAM)m_hWnd );
                return FALSE;

            case VK_ESCAPE:
                ::PostMessage( m_parent, WM_COMMAND, MAKEWPARAM( IDCANCEL, BN_CLICKED ), (LPARAM)m_hWnd );
                return FALSE;

            case VK_TAB:
                ::PostMessage( m_parent, WM_NEXTDLGCTL, 0, 0 );
                return FALSE;
            }

            return DefWindowProc( uMsg, wParam, lParam );
        }

    private:
        HWND m_parent = nullptr;
    };

    CEditWithReturn m_find;
    HWND m_hedit;
    int m_flags;
    std::u8string m_text;
};

} // namespace smp::ui
