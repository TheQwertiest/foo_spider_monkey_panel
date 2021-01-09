#include <stdafx.h>

#include "ui_conf.h"

#include <panel/js_panel_window.h>
#include <utils/array_x.h>

#include <qwr/fb2k_paths.h>
#include <qwr/file_helpers.h>
#include <qwr/final_action.h>
#include <qwr/pfc_helpers_ui.h>

namespace
{

[[maybe_unused]] constexpr int k_File_MenuPosition = 0;
[[maybe_unused]] constexpr int k_Edit_MenuPosition = 1;
constexpr int k_Features_MenuPosition = 2;
constexpr int k_EdgeStyle_MenuPosition = 0;

constexpr auto k_DialogExtFilter = smp::to_array<COMDLG_FILTERSPEC>(
    {
        { L"JavaScript files", L"*.js" },
        { L"Text files", L"*.txt" },
        { L"All files", L"*.*" },
    } );

} // namespace

namespace smp::ui
{

CDialogConf::CDialogConf( smp::panel::js_panel_window* p_parent )
    : m_parent( p_parent )
{
}

LRESULT CDialogConf::OnInitDialog( HWND, LPARAM )
{
    auto& panelSettings = m_parent->GetSettings();

    menu = GetMenu();
    assert( menu.m_hMenu );

    // Get caption text
    m_caption = qwr::pfc_x::uGetWindowText<char8_t>( m_hWnd );

    // Init resize
    DlgResize_Init();

    // Apply window placement
    auto& windowPlacement = panelSettings.windowPlacement;
    if ( windowPlacement.length == 0 )
    {
        windowPlacement.length = sizeof( WINDOWPLACEMENT );

        if ( !GetWindowPlacement( &windowPlacement ) )
        {
            memset( &windowPlacement, 0, sizeof( WINDOWPLACEMENT ) );
        }
    }
    else
    {
        SetWindowPlacement( &windowPlacement );
    }

    // GUID Text
    const std::u8string guid_text = fmt::format( "GUID: {}", pfc::print_guid( panelSettings.guid ) );
    uSetWindowText( GetDlgItem( IDC_STATIC_GUID ), guid_text.c_str() );

    // Edit Control
    sciEditor_.SubclassWindow( GetDlgItem( IDC_EDIT ) );
    sciEditor_.SetScintillaSettings();
    sciEditor_.SetJScript();
    sciEditor_.ReadAPI();
    sciEditor_.SetContent( panelSettings.script.c_str(), true );
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
        (void)menu.CheckMenuRadioItem( ID_EDGESTYLE_NONE, ID_EDGESTYLE_GREY, ID_EDGESTYLE_NONE + static_cast<int>( panelSettings.edgeStyle ), MF_BYCOMMAND );
    }

    // Pseudo Transparent
    if ( m_parent->GetPanelType() == smp::panel::PanelType::CUI )
    {
        (void)menu.CheckMenuItem( ID_PANELFEATURES_PSEUDOTRANSPARENT, panelSettings.isPseudoTransparent ? MF_CHECKED : MF_UNCHECKED );
    }
    else
    {
        (void)menu.CheckMenuItem( ID_PANELFEATURES_PSEUDOTRANSPARENT, MF_UNCHECKED );
        (void)menu.EnableMenuItem( ID_PANELFEATURES_PSEUDOTRANSPARENT, MF_GRAYED );
    }

    // Grab Focus
    (void)menu.CheckMenuItem( ID_PANELFEATURES_GRABFOCUS, panelSettings.shouldGrabFocus ? MF_CHECKED : MF_UNCHECKED );

    return TRUE; // set focus to default control
}

LRESULT CDialogConf::OnCloseCmd( WORD, WORD wID, HWND )
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
            default:
                break;
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

    auto& panelSettings = m_parent->GetSettings();

    if ( menu.GetMenuState( ID_EDGESTYLE_NONE, MF_BYCOMMAND ) & MF_CHECKED )
    {
        panelSettings.edgeStyle = smp::config::EdgeStyle::NO_EDGE;
    }
    else if ( menu.GetMenuState( ID_EDGESTYLE_GREY, MF_BYCOMMAND ) & MF_CHECKED )
    {
        panelSettings.edgeStyle = smp::config::EdgeStyle::GREY_EDGE;
    }
    else if ( menu.GetMenuState( ID_EDGESTYLE_SUNKEN, MF_BYCOMMAND ) & MF_CHECKED )
    {
        panelSettings.edgeStyle = smp::config::EdgeStyle::SUNKEN_EDGE;
    }

    panelSettings.shouldGrabFocus = menu.GetMenuState( ID_PANELFEATURES_GRABFOCUS, MF_BYCOMMAND ) & MF_CHECKED;
    panelSettings.isPseudoTransparent = menu.GetMenuState( ID_PANELFEATURES_PSEUDOTRANSPARENT, MF_BYCOMMAND ) & MF_CHECKED;
    m_parent->update_script( code.data() );

    // Window position
    GetWindowPlacement( &panelSettings.windowPlacement );

    // Save point
    sciEditor_.SetSavePoint();
}

LRESULT CDialogConf::OnNotify( int, LPNMHDR pnmh )
{
    // SCNotification* notification = reinterpret_cast<SCNotification*>( pnmh );

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

LRESULT CDialogConf::OnUwmKeyDown( UINT, WPARAM wParam, LPARAM, BOOL& bHandled )
{
    const auto vk = static_cast<uint32_t>( wParam );
    bHandled = BOOL( ProcessKey( vk ) || sciEditor_.ProcessKey( vk ) );
    return ( bHandled ? 0 : 1 );
}

LRESULT CDialogConf::OnFileSave( WORD, WORD, HWND )
{
    Apply();
    return 0;
}

LRESULT CDialogConf::OnFileImport( WORD, WORD, HWND )
{
    const auto filename_opt = qwr::file::FileDialog( L"Import File", false, guid::dialog_path, k_DialogExtFilter, L"js" );
    if ( !filename_opt || filename_opt->empty() )
    {
        return 0;
    }

    const auto filename = *filename_opt;

    try
    {
        const auto text = qwr::file::ReadFile( filename, CP_UTF8 );
        sciEditor_.SetContent( text.c_str() );
    }
    catch ( const qwr::QwrException& e )
    {
        const std::string errorMsg = fmt::format( "Failed to read file: {}", e.what() );
        (void)uMessageBox( m_hWnd, errorMsg.c_str(), m_caption.c_str(), MB_ICONWARNING | MB_SETFOREGROUND );
    }

    return 0;
}

LRESULT CDialogConf::OnFileExport( WORD, WORD, HWND )
{
    const auto filename_opt = qwr::file::FileDialog( L"Export File", true, guid::dialog_path, k_DialogExtFilter, L"js" );
    if ( !filename_opt || filename_opt->empty() )
    {
        return 0;
    }

    const auto filename = *filename_opt;

    std::u8string text;
    text.resize( sciEditor_.GetTextLength() + 1 );

    sciEditor_.GetText( text.data(), text.size() );
    text.resize( strlen( text.data() ) );

    qwr::file::WriteFile( filename, text );

    return 0;
}

LRESULT CDialogConf::OnEditResetDefault( WORD, WORD, HWND )
{
    sciEditor_.SetContent( smp::config::PanelSettings::GetDefaultScript().c_str() );
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
    const auto path = qwr::path::Component() / L"docs/html/index.html";
    ShellExecute( nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOW );
    return 0;
}

LRESULT CDialogConf::OnAbout( WORD, WORD, HWND )
{
    (void)uMessageBox( m_hWnd, SMP_ABOUT, "About Spider Monkey Panel", MB_SETFOREGROUND );
    return 0;
}

bool CDialogConf::ProcessKey( uint32_t vk )
{
    const int modifiers = ( IsKeyPressed( VK_SHIFT ) ? SCMOD_SHIFT : 0 )
                          | ( IsKeyPressed( VK_CONTROL ) ? SCMOD_CTRL : 0 )
                          | ( IsKeyPressed( VK_MENU ) ? SCMOD_ALT : 0 );

    // Hotkeys
    if ( modifiers == SCMOD_CTRL && vk == 's' )
    {
        Apply();
        return true;
    }

    return false;
}

} // namespace smp::ui
