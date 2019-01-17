#include "stdafx.h"
#include "ui_conf.h"

#include <ui/ui_goto.h>
#include <ui/ui_find.h>
#include <ui/ui_replace.h>
#include <utils/scope_helpers.h>
#include <utils/file_helpers.h>
#include <js_panel_window.h>
#include <component_paths.h>

namespace
{

constexpr int k_File_MenuPosition = 0;
constexpr int k_Edit_MenuPosition = 1;
constexpr int k_Features_MenuPosition = 2;
constexpr int k_EdgeStyle_MenuPosition = 0;

constexpr COMDLG_FILTERSPEC k_DialogExtFilter[3] =
    {
        { L"JavaScript files", L"*.js" },
        { L"Text files", L"*.txt" },
        { L"All files", L"*.*" },
    };

} // namespace

CDialogConf::CDialogConf( smp::panel::js_panel_window* p_parent )
    : m_parent( p_parent )
{
}

CDialogConf::~CDialogConf()
{
    m_hWnd = nullptr;
}

LRESULT CDialogConf::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    menu = GetMenu();
    assert( menu.m_hMenu );

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
    if ( m_parent->GetPanelType() == smp::panel::PanelType::DUI
         && core_version_info_v2::get()->test_version( 1, 4, 0, 0 ) )
    { // disable in default UI fb2k v1.4 and above
        (void)menu.CheckMenuRadioItem( ID_EDGESTYLE_NONE, ID_EDGESTYLE_GREY, ID_EDGESTYLE_NONE, MF_BYCOMMAND );
        (void)menu.GetSubMenu( k_Features_MenuPosition ).EnableMenuItem( k_EdgeStyle_MenuPosition, MF_BYPOSITION | MF_GRAYED );
    }
    else
    {
        (void)menu.CheckMenuRadioItem( ID_EDGESTYLE_NONE, ID_EDGESTYLE_GREY, ID_EDGESTYLE_NONE + static_cast<int>( m_parent->get_edge_style() ), MF_BYCOMMAND );
    }

    // Pseudo Transparent
    if ( m_parent->GetPanelType() == smp::panel::PanelType::CUI )
    {
        (void)menu.CheckMenuItem( ID_PANELFEATURES_PSEUDOTRANSPARENT, m_parent->get_pseudo_transparent() ? MF_CHECKED : MF_UNCHECKED );
    }
    else
    {
        (void)menu.CheckMenuItem( ID_PANELFEATURES_PSEUDOTRANSPARENT, MF_UNCHECKED );
        (void)menu.EnableMenuItem( ID_PANELFEATURES_PSEUDOTRANSPARENT, MF_GRAYED );
    }

    // Grab Focus
    (void)menu.CheckMenuItem( ID_PANELFEATURES_GRABFOCUS, m_parent->get_grab_focus() ? MF_CHECKED : MF_UNCHECKED );

    return TRUE; // set focus to default control
}

LRESULT CDialogConf::OnCloseCmd( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{
    switch ( wID )
    {
    case IDOK:
    {
        Apply();
        EndDialog( IDOK );
        break;
    }
    case IDAPPLY:
    {
        Apply();
        break;
    }
    case IDCANCEL:
    {
        if ( m_editorctrl.GetModify() )
        {
            const int ret = uMessageBox( m_hWnd, "Do you want to apply your changes?", m_caption, MB_ICONWARNING | MB_SETFOREGROUND | MB_YESNOCANCEL );
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
        break;
    }
    default:
    {
        assert( 0 );
    }
    }

    return 0;
}

void CDialogConf::Apply()
{
    // Get script text
    std::vector<char> code( m_editorctrl.GetTextLength() + 1 );
    m_editorctrl.GetText( code.data(), code.size() );

    if ( menu.GetMenuState( ID_EDGESTYLE_NONE, MF_BYCOMMAND ) & MF_CHECKED )
    {
        m_parent->get_edge_style() = smp::config::EdgeStyle::NO_EDGE;
    }
    else if ( menu.GetMenuState( ID_EDGESTYLE_GREY, MF_BYCOMMAND ) & MF_CHECKED )
    {
        m_parent->get_edge_style() = smp::config::EdgeStyle::GREY_EDGE;
    }
    else if ( menu.GetMenuState( ID_EDGESTYLE_SUNKEN, MF_BYCOMMAND ) & MF_CHECKED )
    {
        m_parent->get_edge_style() = smp::config::EdgeStyle::SUNKEN_EDGE;
    }

    m_parent->get_grab_focus() = menu.GetMenuState( ID_PANELFEATURES_GRABFOCUS, MF_BYCOMMAND ) & MF_CHECKED;
    m_parent->get_pseudo_transparent() = menu.GetMenuState( ID_PANELFEATURES_PSEUDOTRANSPARENT, MF_BYCOMMAND ) & MF_CHECKED;
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

LRESULT CDialogConf::OnFileSave( WORD, WORD, HWND )
{
    Apply();
    return 0;
}

LRESULT CDialogConf::OnFileImport( WORD, WORD, HWND )
{
    const pfc::stringcvt::string_utf8_from_os filename( smp::file::FileDialog( L"Import File", false, k_DialogExtFilter, L"js" ).c_str() );
    if ( filename.is_empty() )
    {
        return 0;
    }

    try
    {
        pfc::string8_fast text = smp::file::ReadFile( filename.toString(), CP_UTF8 );
        m_editorctrl.SetContent( text );
    }
    catch ( const smp::SmpException& e )
    {
        std::string errorMsg = e.what();
        errorMsg = "Failed to read file: " + errorMsg;
        (void)uMessageBox( m_hWnd, errorMsg.c_str(), m_caption, MB_ICONWARNING | MB_SETFOREGROUND );
    }

    return 0;
}

LRESULT CDialogConf::OnFileExport( WORD, WORD, HWND )
{
    const pfc::stringcvt::string_utf8_from_os filename( smp::file::FileDialog( L"Export File", true, k_DialogExtFilter, L"js" ).c_str() );
    if ( filename.is_empty() )
    {
        return 0;
    }

    const int len = m_editorctrl.GetTextLength();
    pfc::string8_fast text;

    m_editorctrl.GetText( text.lock_buffer( len ), len + 1 );
    text.unlock_buffer();

    (void)smp::file::WriteFile( filename, text );

    return 0;
}

LRESULT CDialogConf::OnEditResetDefault( WORD, WORD, HWND )
{
    m_editorctrl.SetContent( smp::config::PanelSettings::get_default_script_code() );
    return 0;
}

LRESULT CDialogConf::OnFeaturesEdgeStyle( WORD, WORD wID, HWND )
{
    assert( wID >= ID_EDGESTYLE_NONE && wID <= ID_EDGESTYLE_GREY );
    (void)menu.CheckMenuRadioItem( ID_EDGESTYLE_NONE, ID_EDGESTYLE_GREY, wID, MF_BYCOMMAND );
    return 0;
}

LRESULT CDialogConf::OnFeaturesPseudoTransparent( WORD, WORD, HWND )
{
    auto menuState = menu.GetMenuState( ID_PANELFEATURES_PSEUDOTRANSPARENT, MF_BYCOMMAND );
    if ( !( menuState & MF_GRAYED ) && !( menuState & MF_DISABLED ) )
    {
        (void)menu.CheckMenuItem( ID_PANELFEATURES_PSEUDOTRANSPARENT, ( menuState & MF_CHECKED ) ? MF_UNCHECKED : MF_CHECKED );
    }

    return 0;
}

LRESULT CDialogConf::OnFeaturesGrabFocus( WORD, WORD, HWND )
{
    auto menuState = menu.GetMenuState( ID_PANELFEATURES_GRABFOCUS, MF_BYCOMMAND );
    if ( !( menuState & MF_GRAYED ) && !( menuState & MF_DISABLED ) )
    {
        (void)menu.CheckMenuItem( ID_PANELFEATURES_GRABFOCUS, ( menuState & MF_CHECKED ) ? MF_UNCHECKED : MF_CHECKED );
    }

    return 0;
}

LRESULT CDialogConf::OnHelp( WORD, WORD, HWND )
{
    pfc::stringcvt::string_os_from_utf8 path( smp::get_fb2k_component_path() );
    ShellExecute( 0, L"open", path + L"\\docs\\html\\index.html", 0, 0, SW_SHOW );
    return 0;
}

LRESULT CDialogConf::OnAbout( WORD, WORD, HWND )
{
    (void)uMessageBox( m_hWnd, SMP_ABOUT, "About Spider Monkey Panel", MB_SETFOREGROUND );
    return 0;
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
    (void)uMessageBox( hWnd, buff.get_ptr(), SMP_NAME, MB_ICONINFORMATION | MB_SETFOREGROUND );
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
