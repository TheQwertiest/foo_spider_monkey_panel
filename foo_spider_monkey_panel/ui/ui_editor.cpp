#include <stdafx.h>

#include "ui_editor.h"

#include <panel/js_panel_window.h>
#include <ui/scintilla/sci_config.h>
#include <utils/array_x.h>

#include <qwr/fb2k_paths.h>
#include <qwr/file_helpers.h>
#include <qwr/final_action.h>
#include <qwr/pfc_helpers_ui.h>

namespace
{

constexpr auto k_DialogExtFilter = smp::to_array<COMDLG_FILTERSPEC>( {
    { L"JavaScript files", L"*.js" },
    { L"Text files", L"*.txt" },
    { L"All files", L"*.*" },
} );

WINDOWPLACEMENT g_WindowPlacement{};

} // namespace

namespace smp::ui
{

CEditor::CEditor( std::u8string& text, SaveCallback callback )
    : callback_( callback )
    , text_( text )
{
}

LRESULT CEditor::OnInitDialog( HWND, LPARAM )
{
    menu = GetMenu();
    assert( menu.m_hMenu );

    // Get caption text
    m_caption = qwr::pfc_x::uGetWindowText<char8_t>( m_hWnd );

    // Init resize
    DlgResize_Init();

    // Apply window placement
    if ( !g_WindowPlacement.length )
    {
        WINDOWPLACEMENT tmpPlacement{};
        tmpPlacement.length = sizeof( WINDOWPLACEMENT );

        if ( GetWindowPlacement( &tmpPlacement ) )
        {
            g_WindowPlacement = tmpPlacement;
        }
    }
    else
    {
        SetWindowPlacement( &g_WindowPlacement );
    }

    // Edit Control
    sciEditor_.SubclassWindow( GetDlgItem( IDC_EDIT ) );
    sciEditor_.SetScintillaSettings();
    sciEditor_.SetJScript();
    sciEditor_.ReadAPI();
    sciEditor_.SetContent( text_.c_str(), true );
    sciEditor_.SetSavePoint();

    return TRUE; // set focus to default control
}

LRESULT CEditor::OnCloseCmd( WORD, WORD wID, HWND )
{
    // Window position
    {
        WINDOWPLACEMENT tmpPlacement{};
        tmpPlacement.length = sizeof( WINDOWPLACEMENT );
        if ( GetWindowPlacement( &tmpPlacement ) )
        {
            g_WindowPlacement = tmpPlacement;
        }
    }

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

void CEditor::Apply()
{
    sciEditor_.SetSavePoint();

    std::vector<char> textBuffer( sciEditor_.GetTextLength() + 1 );
    sciEditor_.GetText( textBuffer.data(), textBuffer.size() );
    text_ = textBuffer.data();

    if ( callback_ )
    {
        callback_();
    }
}

LRESULT CEditor::OnNotify( int, LPNMHDR pnmh )
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

LRESULT CEditor::OnUwmKeyDown( UINT, WPARAM wParam, LPARAM, BOOL& bHandled )
{
    const auto vk = (uint32_t)wParam;
    bHandled = BOOL( ProcessKey( vk ) || sciEditor_.ProcessKey( vk ) );
    return ( bHandled ? 0 : 1 );
}

LRESULT CEditor::OnFileSave( WORD, WORD, HWND )
{
    Apply();
    return 0;
}

LRESULT CEditor::OnFileImport( WORD, WORD, HWND )
{
    qwr::file::FileDialogOptions fdOpts{};
    fdOpts.savePathGuid = guid::dialog_path;
    fdOpts.filterSpec.assign( k_DialogExtFilter.begin(), k_DialogExtFilter.end() );
    fdOpts.defaultExtension = L"js";

    const auto filename = qwr::file::FileDialog( L"Import File", false, fdOpts );
    if ( !filename || filename->empty() )
    {
        return 0;
    }

    try
    {
        const auto text = qwr::file::ReadFile( *filename, CP_UTF8 );
        sciEditor_.SetContent( text.c_str() );
    }
    catch ( const qwr::QwrException& e )
    {
        const auto errorMsg = fmt::format( "Failed to read file: {}", e.what() );
        (void)uMessageBox( m_hWnd, errorMsg.c_str(), m_caption.c_str(), MB_ICONWARNING | MB_SETFOREGROUND );
    }

    return 0;
}

LRESULT CEditor::OnFileExport( WORD, WORD, HWND )
{
    qwr::file::FileDialogOptions fdOpts{};
    fdOpts.savePathGuid = guid::dialog_path;
    fdOpts.filterSpec.assign( k_DialogExtFilter.begin(), k_DialogExtFilter.end() );
    fdOpts.defaultExtension = L"js";

    const auto filename = qwr::file::FileDialog( L"Export File", true, fdOpts );
    if ( !filename || filename->empty() )
    {
        return 0;
    }

    std::u8string text;
    text.resize( sciEditor_.GetTextLength() + 1 );

    sciEditor_.GetText( text.data(), text.size() );
    text.resize( strlen( text.data() ) );

    (void)qwr::file::WriteFile( *filename, text );

    return 0;
}

LRESULT CEditor::OnHelp( WORD, WORD, HWND )
{
    const auto path = qwr::path::Component() / L"docs/html/index.html";
    ShellExecute( nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOW );
    return 0;
}

LRESULT CEditor::OnAbout( WORD, WORD, HWND )
{
    (void)uMessageBox( m_hWnd, SMP_ABOUT, "About Spider Monkey Panel", MB_SETFOREGROUND );
    return 0;
}

bool CEditor::ProcessKey( uint32_t vk )
{
    const int modifiers = ( IsKeyPressed( VK_SHIFT ) ? SCMOD_SHIFT : 0 )
                          | ( IsKeyPressed( VK_CONTROL ) ? SCMOD_CTRL : 0 )
                          | ( IsKeyPressed( VK_MENU ) ? SCMOD_ALT : 0 );

    // Hotkeys
    if ( modifiers == SCMOD_CTRL && vk == 'S' )
    {
        Apply();
        return true;
    }

    return false;
}

} // namespace smp::ui
