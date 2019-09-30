#include <stdafx.h>

#include "ui_conf.h"

#include <ui/ui_goto.h>
#include <utils/file_helpers.h>
#include <utils/scope_helpers.h>

#include <component_paths.h>
#include <js_panel_window.h>

namespace
{

constexpr int k_File_MenuPosition = 0;
constexpr int k_Edit_MenuPosition = 1;
constexpr int k_Features_MenuPosition = 2;
constexpr int k_EdgeStyle_MenuPosition = 0;

constexpr COMDLG_FILTERSPEC k_DialogExtFilter[3] = {
    { L"JavaScript files", L"*.js" },
    { L"Text files", L"*.txt" },
    { L"All files", L"*.*" },
};

} // namespace

namespace smp::ui
{

CDialogConf::CDialogConf( smp::panel::js_panel_window* p_parent )
    : m_parent( p_parent )
{
}

LRESULT CDialogConf::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    menu = GetMenu();
    assert( menu.m_hMenu );

    // Get caption text
    m_caption = smp::pfc_x::uGetWindowText<char8_t>( m_hWnd );

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
    const std::u8string guid_text = fmt::format( "GUID: {}", pfc::print_guid( m_parent->get_config_guid() ) );
    uSetWindowText( GetDlgItem( IDC_STATIC_GUID ), guid_text.c_str() );

    // Edit Control
    sciEditor_.SubclassWindow( GetDlgItem( IDC_EDIT ) );
    sciEditor_.SetScintillaSettings();
    sciEditor_.SetJScript();
    sciEditor_.ReadAPI();
    sciEditor_.SetContent( m_parent->get_script_code().c_str(), true );
    sciEditor_.SetSavePoint();

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
        if ( sciEditor_.GetModify() )
        {
            const int ret = uMessageBox( m_hWnd, "Do you want to apply your changes?", m_caption.c_str(), MB_ICONWARNING | MB_SETFOREGROUND | MB_YESNOCANCEL );
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
    std::vector<char> code( sciEditor_.GetTextLength() + 1 );
    sciEditor_.GetText( code.data(), code.size() );

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
    sciEditor_.SetSavePoint();
}

LRESULT CDialogConf::OnNotify( int idCtrl, LPNMHDR pnmh )
{
    SCNotification* notification = reinterpret_cast<SCNotification*>( pnmh );

    switch ( pnmh->code )
    {
    case SCN_SAVEPOINTLEFT:
    { // dirty
        uSetWindowText( m_hWnd, ( m_caption + " *" ).c_str() );
        break;
    }
    case SCN_SAVEPOINTREACHED:
    { // not dirty
        uSetWindowText( m_hWnd, m_caption.c_str() );
        break;
    }
    }

    SetMsgHandled( FALSE );
    return 0;
}

bool CDialogConf::ProcessKey( uint32_t vk )
{
    const int modifiers = ( IsKeyPressed( VK_SHIFT ) ? SCMOD_SHIFT : 0 )
                          | ( IsKeyPressed( VK_CONTROL ) ? SCMOD_CTRL : 0 )
                          | ( IsKeyPressed( VK_MENU ) ? SCMOD_ALT : 0 );

    // Hotkeys
    if ( modifiers == SCMOD_CTRL )
    {
        switch ( vk )
        {
        case 'S':
        {
            Apply();
            return true;
        }
        }
    }

    return false;
}

LRESULT CDialogConf::OnUwmKeyDown( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    const uint32_t vk = (uint32_t)wParam;
    bHandled = ( ProcessKey( vk ) || sciEditor_.ProcessKey( vk ) );
    return ( bHandled ? 0 : 1 );
}

LRESULT CDialogConf::OnFileSave( WORD, WORD, HWND )
{
    Apply();
    return 0;
}

LRESULT CDialogConf::OnFileImport( WORD, WORD, HWND )
{
    const auto filename = smp::unicode::ToU8( smp::file::FileDialog( L"Import File", false, k_DialogExtFilter, L"js" ) );
    if ( filename.empty() )
    {
        return 0;
    }

    try
    {
        const auto text = smp::file::ReadFile( filename, CP_UTF8 );
        sciEditor_.SetContent( text.c_str() );
    }
    catch ( const smp::SmpException& e )
    {
        const std::string errorMsg = fmt::format( "Failed to read file: {}", e.what() );
        (void)uMessageBox( m_hWnd, errorMsg.c_str(), m_caption.c_str(), MB_ICONWARNING | MB_SETFOREGROUND );
    }

    return 0;
}

LRESULT CDialogConf::OnFileExport( WORD, WORD, HWND )
{
    const std::wstring filename( smp::file::FileDialog( L"Export File", true, k_DialogExtFilter, L"js" ).c_str() );
    if ( filename.empty() )
    {
        return 0;
    }

    std::u8string text;
    text.resize( sciEditor_.GetTextLength() + 1 );

    sciEditor_.GetText( text.data(), text.size() );
    text.resize( strlen( text.data() ) );

    (void)smp::file::WriteFile( filename.c_str(), text );

    return 0;
}

LRESULT CDialogConf::OnEditResetDefault( WORD, WORD, HWND )
{
    sciEditor_.SetContent( smp::config::PanelSettings::get_default_script_code().c_str() );
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
    const auto path = smp::unicode::ToWide( smp::get_fb2k_component_path() ) + L"\\docs\\html\\index.html";
    ShellExecute( 0, L"open", path.c_str(), 0, 0, SW_SHOW );
    return 0;
}

LRESULT CDialogConf::OnAbout( WORD, WORD, HWND )
{
    (void)uMessageBox( m_hWnd, SMP_ABOUT, "About Spider Monkey Panel", MB_SETFOREGROUND );
    return 0;
}

} // namespace smp::ui
