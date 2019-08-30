#include <stdafx.h>
#include "ui_replace.h"
#include "ui_conf.h"

namespace smp::ui
{

CDialogReplace::CDialogReplace( HWND p_hedit )
    : m_hedit( p_hedit )
    , m_flags( 0 )
    , m_havefound( false )
{
}

LRESULT CDialogReplace::OnFindNext( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{
    if ( m_text.empty() )
        return 0;

    m_havefound = CDialogConf::FindNext( m_hWnd, m_hedit, m_flags, m_text.c_str() );
    return 0;
}

LRESULT CDialogReplace::OnEditFindWhatEnChange( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{
    m_text = smp::pfc_x::uGetWindowText<char8_t>( GetDlgItem( IDC_EDIT_FINDWHAT ) );
    return 0;
}

LRESULT CDialogReplace::OnFlagCommand( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{
    bool check = uButton_GetCheck( m_hWnd, wID );
    int flag = 0;

    switch ( wID )
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

    if ( check )
        m_flags |= flag;
    else
        m_flags &= ~flag;

    return 0;
}

LRESULT CDialogReplace::OnCancel( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{
    //DestroyWindow();
    ShowWindow( SW_HIDE );
    return 0;
}

void CDialogReplace::OnFinalMessage( HWND hWnd )
{
    modeless_dialog_manager::g_remove( m_hWnd );
    delete this;
}

LRESULT CDialogReplace::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    modeless_dialog_manager::g_add( m_hWnd );
    DlgResize_Init();
    m_replace.SubclassWindow( GetDlgItem( IDC_EDIT_FINDWHAT ), m_hWnd );
    m_find.SubclassWindow( GetDlgItem( IDC_EDIT_REPLACE ), m_hWnd );
    return TRUE; // set focus to default control
}

LRESULT CDialogReplace::OnEditReplaceEnChange( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{
    m_reptext = smp::pfc_x::uGetWindowText<char8_t>( GetDlgItem( IDC_EDIT_REPLACE ) );
    return 0;
}

LRESULT CDialogReplace::OnReplace( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{
    if ( m_havefound )
    {
        CHARRANGE cr = GetSelection();

        SendMessage( m_hedit, SCI_SETTARGETSTART, cr.cpMin, 0 );
        SendMessage( m_hedit, SCI_SETTARGETEND, cr.cpMax, 0 );
        SendMessage( m_hedit, SCI_REPLACETARGET, m_reptext.length(), (LPARAM)m_reptext.c_str() );
        SendMessage( m_hedit, SCI_SETSEL, cr.cpMin + m_reptext.length(), cr.cpMin );
        m_havefound = false;
    }

    // Find Next
    OnFindNext( 0, 0, 0 );
    return 0;
}

LRESULT CDialogReplace::OnReplaceall( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{
    // Reset target position
    SendMessage( m_hedit, SCI_BEGINUNDOACTION, 0, 0 );
    SendMessage( m_hedit, SCI_SETTARGETSTART, 0, 0 );
    SendMessage( m_hedit, SCI_SETTARGETEND, 0, 0 );

    while ( true )
    {
        int start_pos = SendMessage( m_hedit, SCI_GETTARGETEND, 0, 0 );
        int end_pos = SendMessage( m_hedit, SCI_GETLENGTH, 0, 0 );

        SendMessage( m_hedit, SCI_SETTARGETSTART, start_pos, 0 );
        SendMessage( m_hedit, SCI_SETTARGETEND, end_pos, 0 );
        SendMessage( m_hedit, SCI_SETSEARCHFLAGS, m_flags, 0 );

        int occurance = SendMessage( m_hedit, SCI_SEARCHINTARGET, m_text.length(), (LPARAM)m_text.c_str() );

        if ( occurance == -1 )
        {
            MessageBeep( MB_ICONINFORMATION );
            break;
        }

        SendMessage( m_hedit, SCI_REPLACETARGET, m_reptext.length(), (LPARAM)m_reptext.c_str() );
        SendMessage( m_hedit, SCI_SETSEL, occurance + m_reptext.length(), occurance );
    }

    SendMessage( m_hedit, SCI_ENDUNDOACTION, 0, 0 );
    return 0;
}

CHARRANGE CDialogReplace::GetSelection()
{
    CHARRANGE cr;

    cr.cpMin = SendMessage( m_hedit, SCI_GETSELECTIONSTART, 0, 0 );
    cr.cpMax = SendMessage( m_hedit, SCI_GETSELECTIONEND, 0, 0 );
    return cr;
}

} // namespace smp::ui
