#include <stdafx.h>

#include "ui_editor.h"

#include <panel/js_panel_window.h>
#include <ui/scintilla/sci_config.h>
#include <ui/ui_editor_config.h>

#include <component_paths.h>

#include <qwr/fb2k_paths.h>
#include <qwr/file_helpers.h>
#include <qwr/pfc_helpers_ui.h>
#include <qwr/ui_centered_message_box.h>

namespace
{

constexpr auto k_DialogExtFilter = std::to_array<COMDLG_FILTERSPEC>( {
    { L"JavaScript files", L"*.js" },
    { L"Text files", L"*.txt" },
    { L"All files", L"*.*" },
} );

WINDOWPLACEMENT g_WindowPlacement{};

} // namespace

namespace smp::ui
{

CEditor::CEditor( const qwr::u8string& caption, qwr::u8string& text, SaveCallback callback )
    : callback_( callback )
    , text_( text )
    , caption_( caption )
{
}

LRESULT CEditor::OnInitDialog( HWND, LPARAM )
{
    menu = GetMenu();
    assert( menu.m_hMenu );

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
    ReloadProperties();
    sciEditor_.SetContent( text_.c_str(), true );
    sciEditor_.SetSavePoint();

    UpdateUiElements();

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
    case ID_APP_EXIT:
    {
        if ( sciEditor_.GetModify() )
        {
            const int ret = qwr::ui::MessageBoxCentered( *this,
                                                         L"Do you want to apply your changes?",
                                                         qwr::unicode::ToWide( caption_ ).c_str(),
                                                         MB_ICONWARNING | MB_SETFOREGROUND | MB_YESNOCANCEL );
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

        EndDialog( wID );
        break;
    }
    default:
    {
        assert( 0 );
    }
    }

    return 0;
}

LRESULT CEditor::OnNotify( int, LPNMHDR pnmh )
{
    // SCNotification* notification = reinterpret_cast<SCNotification*>( pnmh );

    switch ( pnmh->code )
    {
    case SCN_SAVEPOINTLEFT:
    { // dirty
        isDirty_ = true;
        UpdateUiElements();
        break;
    }
    case SCN_SAVEPOINTREACHED:
    { // not dirty
        isDirty_ = false;
        UpdateUiElements();
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
        (void)qwr::ui::MessageBoxCentered( *this,
                                           qwr::unicode::ToWide( errorMsg ).c_str(),
                                           qwr::unicode::ToWide( caption_ ).c_str(),
                                           MB_ICONWARNING | MB_SETFOREGROUND );
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

    qwr::u8string text;
    text.resize( sciEditor_.GetTextLength() + 1 );

    sciEditor_.GetText( text.data(), text.size() );
    text.resize( strlen( text.data() ) );

    (void)qwr::file::WriteFile( *filename, text );

    return 0;
}

LRESULT CEditor::OnOptionProperties( WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/ )
{
    CDialogEditorConfig config;
    config.DoModal( m_hWnd );

    ReloadProperties();

    return 0;
}

LRESULT CEditor::OnHelp( WORD, WORD, HWND )
{
    ShellExecute( nullptr, L"open", path::JsDocsIndex().c_str(), nullptr, nullptr, SW_SHOW );
    return 0;
}

LRESULT CEditor::OnAbout( WORD, WORD, HWND )
{
    (void)qwr::ui::MessageBoxCentered( *this, TEXT( SMP_ABOUT ), L"About Spider Monkey Panel", MB_SETFOREGROUND );
    return 0;
}

void CEditor::ReloadProperties()
{
    sciEditor_.ReloadScintillaSettings();
    sciEditor_.SetJScript();
    sciEditor_.ReadAPI();
}

void CEditor::UpdateUiElements()
{
    if ( isDirty_ )
    {
        CButton( GetDlgItem( IDAPPLY ) ).EnableWindow( TRUE );
        uSetWindowText( m_hWnd, ( caption_ + " *" ).c_str() );
    }
    else
    {
        CButton( GetDlgItem( IDAPPLY ) ).EnableWindow( FALSE );
        uSetWindowText( m_hWnd, caption_.c_str() );
    }
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

} // namespace smp::ui
