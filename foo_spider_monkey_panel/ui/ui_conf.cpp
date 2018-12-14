#include "stdafx.h"
#include "ui_conf.h"

#include <ui/ui_goto.h>
#include <ui/ui_find.h>
#include <ui/ui_replace.h>
#include <js_panel_window.h>
#include <helpers.h>

LRESULT CDialogConf::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    // Get caption text
    uGetWindowText( m_hWnd, m_caption );

    // Init resize
    DlgResize_Init();

    // Apply window placement
    if ( m_parent->get_windowplacement().length == 0 )
    {
        m_parent->get_windowplacement().length = sizeof( WINDOWPLACEMENT );

        if ( !GetWindowPlacement( &m_parent->get_windowplacement() ) )
        {
            memset( &m_parent->get_windowplacement(), 0, sizeof( WINDOWPLACEMENT ) );
        }
    }
    else
    {
        SetWindowPlacement( &m_parent->get_windowplacement() );
    }

    // GUID Text
    pfc::string8 guid_text = "GUID: ";
    guid_text += pfc::print_guid( m_parent->get_config_guid() );
    uSetWindowText( GetDlgItem( IDC_STATIC_GUID ), guid_text );

    // Edit Control
    m_editorctrl.SubclassWindow( GetDlgItem( IDC_EDIT ) );
    m_editorctrl.SetScintillaSettings();
    m_editorctrl.SetJScript();
    m_editorctrl.ReadAPI();
    m_editorctrl.SetContent( m_parent->get_script_code(), true );
    m_editorctrl.SetSavePoint();

    // Edge Style
    HWND combo_edge = GetDlgItem( IDC_COMBO_EDGE );
    ComboBox_AddString( combo_edge, _T("None") );
    ComboBox_AddString( combo_edge, _T("Sunken") );
    ComboBox_AddString( combo_edge, _T("Grey") );

    if ( core_version_info_v2::get()->test_version( 1, 4, 0, 0 ) && m_parent->GetPanelType() == smp::panel::PanelType::DUI )
    {
        // disable in default UI fb2k v1.4 and above
        ComboBox_SetCurSel( combo_edge, 0 );
        GetDlgItem( IDC_COMBO_EDGE ).EnableWindow( false );
    }
    else
    {
        ComboBox_SetCurSel( combo_edge, m_parent->get_edge_style() );
    }

    // Pseudo Transparent
    if ( m_parent->GetPanelType() == smp::panel::PanelType::CUI )
    {
        uButton_SetCheck( m_hWnd, IDC_CHECK_PSEUDO_TRANSPARENT, m_parent->get_pseudo_transparent() );
    }
    else
    {
        uButton_SetCheck( m_hWnd, IDC_CHECK_PSEUDO_TRANSPARENT, false );
        GetDlgItem( IDC_CHECK_PSEUDO_TRANSPARENT ).EnableWindow( false );
    }

    // Grab Focus
    uButton_SetCheck( m_hWnd, IDC_CHECK_GRABFOCUS, m_parent->get_grab_focus() );

    return TRUE; // set focus to default control
}

LRESULT CDialogConf::OnCloseCmd( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{
    switch ( wID )
    {
    case IDOK:
        Apply();
        EndDialog( IDOK );
        break;

    case IDAPPLY:
        Apply();
        break;

    case IDCANCEL:
        if ( m_editorctrl.GetModify() )
        {
            int ret = uMessageBox( m_hWnd, "Do you want to apply your changes?", m_caption, MB_ICONWARNING | MB_SETFOREGROUND | MB_YESNOCANCEL );

            switch ( ret )
            {
            case IDYES:
                Apply();
                EndDialog( IDOK );
                break;

            case IDCANCEL:
                return 0;
            }
        }

        EndDialog( IDCANCEL );
    }

    return 0;
}

void CDialogConf::OnResetDefault()
{
    m_editorctrl.SetContent( smp::config::PanelSettings::get_default_script_code() );
}

void CDialogConf::OnResetCurrent()
{
    m_editorctrl.SetContent( m_parent->get_script_code() );
}

void CDialogConf::OnImport()
{
    pfc::string8 filename;

    if ( uGetOpenFileName( m_hWnd, "JavaScript files|*.js|Text files|*.txt|All files|*.*", 0, "js", "Import from", nullptr, filename, FALSE ) )
    {
        // Open file
        pfc::string8_fast text;
        helpers::read_file( filename, text );
        m_editorctrl.SetContent( text );
    }
}

void CDialogConf::OnExport()
{
    pfc::string8 filename;

    if ( uGetOpenFileName( m_hWnd, "JavaScript files|*.js|Text files|*.txt|All files|*.*", 0, "js", "Save as", nullptr, filename, TRUE ) )
    {
        int len = m_editorctrl.GetTextLength();
        pfc::string8_fast text;

        m_editorctrl.GetText( text.lock_buffer( len ), len + 1 );
        text.unlock_buffer();

        helpers::write_file( filename, text );
    }
}

LRESULT CDialogConf::OnTools( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{
    enum
    {
        kImport = 1,
        kExport,
        kResetDefault,
        kResetCurrent,
    };

    HMENU menu = CreatePopupMenu();
    AppendMenu( menu, MF_STRING, kImport, _T("&Import") );
    AppendMenu( menu, MF_STRING, kExport, _T("E&xport") );
    AppendMenu( menu, MF_SEPARATOR, 0, 0 );
    AppendMenu( menu, MF_STRING, kResetDefault, _T("Reset &Default") );
    AppendMenu( menu, MF_STRING, kResetCurrent, _T("Reset &Current") );

    RECT rc = { 0 };
    ::GetWindowRect(::GetDlgItem( m_hWnd, IDC_TOOLS ), &rc );

    int ret = TrackPopupMenu( menu, TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, rc.left, rc.bottom, 0, m_hWnd, 0 );

    switch ( ret )
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

    DestroyMenu( menu );
    return 0;
}

void CDialogConf::Apply()
{
    // Get script text
    std::vector<char> code( m_editorctrl.GetTextLength() + 1 );
    m_editorctrl.GetText( code.data(), code.size() );

    m_parent->get_edge_style() = static_cast<smp::config::EdgeStyle>( ComboBox_GetCurSel( GetDlgItem( IDC_COMBO_EDGE ) ) );
    m_parent->get_grab_focus() = uButton_GetCheck( m_hWnd, IDC_CHECK_GRABFOCUS );
    m_parent->get_pseudo_transparent() = uButton_GetCheck( m_hWnd, IDC_CHECK_PSEUDO_TRANSPARENT );
    m_parent->update_script( code.data() );

    // Window position
    GetWindowPlacement( &m_parent->get_windowplacement() );

    // Save point
    m_editorctrl.SetSavePoint();
}

LRESULT CDialogConf::OnNotify( int idCtrl, LPNMHDR pnmh )
{
    SCNotification* notification = reinterpret_cast<SCNotification*>( pnmh );

    switch ( pnmh->code )
    {
    case SCN_SAVEPOINTLEFT:
    { // dirty
        pfc::string8 caption = m_caption;

        caption += " *";
        uSetWindowText( m_hWnd, caption );
        break;
    }
    case SCN_SAVEPOINTREACHED:
    { // not dirty
        uSetWindowText( m_hWnd, m_caption );
        break;
    }
    }

    SetMsgHandled( FALSE );
    return 0;
}

bool CDialogConf::MatchShortcuts( unsigned vk )
{
    int modifiers = ( IsKeyPressed( VK_SHIFT ) ? SCMOD_SHIFT : 0 )
                    | ( IsKeyPressed( VK_CONTROL ) ? SCMOD_CTRL : 0 )
                    | ( IsKeyPressed( VK_MENU ) ? SCMOD_ALT : 0 );

    // Hotkeys
    if ( modifiers == SCMOD_CTRL )
    {
        switch ( vk )
        {
        case 'F':
        {
            OpenFindDialog();
            return true;
        }
        case 'H':
        {
            if ( !m_dlgreplace )
            {
                m_dlgreplace = new CDialogReplace( GetDlgItem( IDC_EDIT ) );

                if ( !m_dlgreplace || !m_dlgreplace->Create( m_hWnd ) )
                {
                    break;
                }
            }

            m_dlgreplace->ShowWindow( SW_SHOW );
            m_dlgreplace->SetFocus();

            return true;
        }
        case 'G':
        {
            modal_dialog_scope scope( m_hWnd );
            CDialogGoto dlg( GetDlgItem( IDC_EDIT ) );
            dlg.DoModal( m_hWnd );

            return true;
        }
        case 'S':
        {
            Apply();
            return true;
        }
        }
    }
    else if ( modifiers == 0 )
    {
        if ( vk == VK_F3 )
        {
            // Find next one
            if ( !m_lastSearchText.is_empty() )
            {
                FindNext( m_hWnd, m_editorctrl.m_hWnd, m_lastFlags, m_lastSearchText );
            }
            else
            {
                OpenFindDialog();
            }
        }
    }
    else if ( modifiers == SCMOD_SHIFT )
    {
        if ( vk == VK_F3 )
        {
            // Find previous one
            if ( !m_lastSearchText.is_empty() )
            {
                FindPrevious( m_hWnd, m_editorctrl.m_hWnd, m_lastFlags, m_lastSearchText );
            }
            else
            {
                OpenFindDialog();
            }
        }
    }

    return false;
}

LRESULT CDialogConf::OnUwmKeyDown( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    return MatchShortcuts( wParam );
}

LRESULT CDialogConf::OnUwmFindTextChanged( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    m_lastFlags = wParam;
    m_lastSearchText = reinterpret_cast<const char*>( lParam );
    return 0;
}

bool CDialogConf::FindNext( HWND hWnd, HWND hWndEdit, unsigned flags, const char* which )
{
    ::SendMessage(::GetAncestor( hWndEdit, GA_PARENT ), static_cast<UINT>( smp::MiscMessage::find_text_changed ), flags, reinterpret_cast<LPARAM>( which ) );

    SendMessage( hWndEdit, SCI_CHARRIGHT, 0, 0 );
    SendMessage( hWndEdit, SCI_SEARCHANCHOR, 0, 0 );
    int pos = ::SendMessage( hWndEdit, SCI_SEARCHNEXT, flags, reinterpret_cast<LPARAM>( which ) );
    return FindResult( hWnd, hWndEdit, pos, which );
}

bool CDialogConf::FindPrevious( HWND hWnd, HWND hWndEdit, unsigned flags, const char* which )
{
    ::SendMessage(::GetAncestor( hWndEdit, GA_PARENT ), static_cast<UINT>( smp::MiscMessage::find_text_changed ), flags, reinterpret_cast<LPARAM>( which ) );

    SendMessage( hWndEdit, SCI_SEARCHANCHOR, 0, 0 );
    int pos = ::SendMessage( hWndEdit, SCI_SEARCHPREV, flags, reinterpret_cast<LPARAM>( which ) );
    return FindResult( hWnd, hWndEdit, pos, which );
}

bool CDialogConf::FindResult( HWND hWnd, HWND hWndEdit, int pos, const char* which )
{
    if ( pos != -1 )
    { // Scroll to view
        ::SendMessage( hWndEdit, SCI_SCROLLCARET, 0, 0 );
        return true;
    }

    pfc::string8 buff = "Cannot find \"";
    buff += which;
    buff += "\"";
    uMessageBox( hWnd, buff.get_ptr(), SMP_NAME, MB_ICONINFORMATION | MB_SETFOREGROUND );
    return false;
}

void CDialogConf::OpenFindDialog()
{
    if ( !m_dlgfind )
    { // Create it on request.
        m_dlgfind = new CDialogFind( GetDlgItem( IDC_EDIT ) );
        m_dlgfind->Create( m_hWnd );
    }

    m_dlgfind->ShowWindow( SW_SHOW );
    m_dlgfind->SetFocus();
}
