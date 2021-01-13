#include <stdafx.h>

#include "ui_conf_tab_package.h"

#include <config/package_utils.h>
#include <fb2k/config.h>
#include <ui/ui_conf.h>
#include <ui/ui_input_box.h>
#include <utils/edit_text.h>

#include <qwr/error_popup.h>
#include <qwr/file_helpers.h>
#include <qwr/winapi_error_helpers.h>

namespace fs = std::filesystem;

// TODO: add `New` button
// TODO: display package id

namespace smp::ui
{

CConfigTabPackage::CConfigTabPackage( CDialogConf& parent, config::ParsedPanelSettings& settings )
    : parent_( parent )
    , settings_( settings )
    , packagePath_( config::GetPackagePath( settings ) )
    , isSample_( settings.isSample )
    , mainScriptPath_( *settings.scriptPath )
    , scriptName_( settings.scriptName )
    , scriptVersion_( settings.scriptVersion )
    , scriptAuthor_( settings.scriptAuthor )
    , scriptDescription_( settings.scriptDescription )
    , shouldGrabFocus_( settings.shouldGrabFocus )
    , enableDragDrop_( settings.enableDragDrop )
    , focusedFile_( mainScriptPath_ )
    , ddx_( { qwr::ui::CreateUiDdx<qwr::ui::UiDdx_TextEdit>( scriptName_, IDC_EDIT_PACKAGE_NAME ),
              qwr::ui::CreateUiDdx<qwr::ui::UiDdx_TextEdit>( scriptVersion_, IDC_EDIT_PACKAGE_VERSION ),
              qwr::ui::CreateUiDdx<qwr::ui::UiDdx_TextEdit>( scriptAuthor_, IDC_EDIT_PACKAGE_AUTHOR ),
              qwr::ui::CreateUiDdx<qwr::ui::UiDdx_TextEdit>( scriptDescription_, IDC_EDIT_PACKAGE_DESCRIPTION ),
              qwr::ui::CreateUiDdx<qwr::ui::UiDdx_CheckBox>( shouldGrabFocus_, IDC_CHECK_SHOULD_GRAB_FOCUS ),
              qwr::ui::CreateUiDdx<qwr::ui::UiDdx_CheckBox>( shouldGrabFocus_, IDC_CHECK_ENABLE_DRAG_N_DROP ),
              qwr::ui::CreateUiDdx<qwr::ui::UiDdx_ListBox>( focusedFileIdx_, IDC_LIST_PACKAGE_FILES ) } )
{
}

HWND CConfigTabPackage::CreateTab( HWND hParent )
{
    return Create( hParent );
}

CDialogImplBase& CConfigTabPackage::Dialog()
{
    return *this;
}

const wchar_t* CConfigTabPackage::Name() const
{
    return L"Package";
}

bool CConfigTabPackage::HasChanged()
{
    return false;
}

void CConfigTabPackage::Apply()
{
}

void CConfigTabPackage::Revert()
{
}

BOOL CConfigTabPackage::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    for ( auto& ddx: ddx_ )
    {
        ddx->SetHwnd( m_hWnd );
    }

    InitializeFilesListBox();
    DoFullDdxToUi();

    suppressUiDdx_ = false;

    return TRUE; // set focus to default control
}

void CConfigTabPackage::OnDdxUiChange( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    if ( suppressUiDdx_ )
    {
        return;
    }

    {
        auto it = ranges::find_if( ddx_, [nID]( auto& ddx ) {
            return ddx->IsMatchingId( nID );
        } );

        if ( ddx_.end() != it )
        {
            ( *it )->ReadFromUi();
        }
    }

    if ( nID == IDC_LIST_PACKAGE_FILES )
    {
        assert( focusedFileIdx_ < static_cast<int>( files_.size() ) );
        focusedFile_ = files_[focusedFileIdx_];
    }

    UpdateUiButtons();

    parent_.OnDataChanged();
}

void CConfigTabPackage::OnAddFile( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    qwr::file::FileDialogOptions fdOpts{};
    fdOpts.savePathGuid = guid::dialog_path;
    fdOpts.savePathGuid = guid::dialog_path;
    fdOpts.filterSpec.assign( {
        { L"JavaScript files", L"*.js" },
        { L"All files", L"*.*" },
    } );
    fdOpts.defaultExtension = L"js";

    const auto pathOpt = qwr::file::FileDialog( L"Add file", false, fdOpts );
    if ( !pathOpt )
    {
        return;
    }

    try
    {
        const fs::path path( *pathOpt );
        const fs::path newPath = ( path.extension() == ".js"
                                       ? packagePath_ / "scripts" / path.filename()
                                       : packagePath_ / "assets" / path.filename() );

        const auto it = ranges::find( files_, newPath );
        if ( it != files_.cend() )
        {
            // TODO: ask confirmation and rewrite
        }
        else
        {
            fs::create_directories( newPath.parent_path() );
            fs::copy( path, newPath );
            files_.emplace_back( newPath );
        }
        focusedFile_ = newPath;

        UpdateListBoxFromData();
        DoFullDdxToUi();
    }
    catch ( const fs::filesystem_error& e )
    {
        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, qwr::unicode::ToU8_FromAcpToWide( e.what() ) );
    }
    catch ( const qwr::QwrException& e )
    {
        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, e.what() );
    }
}

void CConfigTabPackage::OnRemoveFile( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    if ( files_.empty() )
    {
        return;
    }

    assert( static_cast<size_t>( focusedFileIdx_ ) < files_.size() );
    try
    {
        if ( fs::exists( files_[focusedFileIdx_] ) )
        {
            fs::remove( files_[focusedFileIdx_] );
        }
    }
    catch ( const fs::filesystem_error& e )
    {
        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, e.what() );
    }

    files_.erase( files_.cbegin() + focusedFileIdx_ );
    focusedFile_ = files_[focusedFileIdx_ - 1];

    UpdateListBoxFromData();
    UpdateUiButtons();
    DoFullDdxToUi();
}

void CConfigTabPackage::OnRenameFile( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    assert( static_cast<size_t>( focusedFileIdx_ ) < files_.size() );

    auto& filepath = files_[focusedFileIdx_];

    CInputBox dlg( "Enter new file name", "Rename file", filepath.filename().u8string().c_str() );
    if ( dlg.DoModal( m_hWnd ) != IDOK )
    {
        return;
    }

    try
    {
        const auto newFilePath = filepath.parent_path() / fs::u8path( dlg.GetValue() );
        fs::rename( filepath, newFilePath );
        filepath = newFilePath;

        focusedFile_ = filepath;

        UpdateListBoxFromData();
        DoFullDdxToUi();
    }
    catch ( const fs::filesystem_error& e )
    {
        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, qwr::unicode::ToU8_FromAcpToWide( e.what() ) );
    }
}

void CConfigTabPackage::OnOpenContainingFolder( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    const std::wstring arg = [&] {
        if ( files_.empty() )
        {
            return std::wstring{};
        }
        else
        {
            std::wstring tmp = files_[focusedFileIdx_].wstring();
            return L"\"" + tmp + L"\"";
        }
    }();

    try
    {
        const auto hInstance = ShellExecute( nullptr,
                                             L"explore",
                                             packagePath_.wstring().c_str(),
                                             ( arg.empty() ? nullptr : arg.c_str() ),
                                             nullptr,
                                             SW_SHOWNORMAL );
        if ( (int)hInstance < 32 )
        { // As per WinAPI
            qwr::error::CheckWin32( (int)hInstance, "ShellExecute" );
        }
    }
    catch ( const fs::filesystem_error& e )
    {
        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, qwr::unicode::ToU8_FromAcpToWide( e.what() ) );
    }
    catch ( const qwr::QwrException& e )
    {
        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, e.what() );
    }
}

void CConfigTabPackage::OnEditScript( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    try
    {
        if ( isSample_ )
        {
            const int iRet = MessageBox(
                L"Are you sure?\n\n"
                L"You are trying to edit a sample script.\n"
                L"Any changes performed to the script will be applied to every panel that are using this sample.\n"
                L"These changes will also be lost when updating the component.",
                L"Editing script",
                MB_YESNO );
            if ( iRet != IDYES )
            {
                return;
            }
        }

        const auto filePath = files_[focusedFileIdx_];
        qwr::QwrException::ExpectTrue( fs::exists( filePath ), "Script is missing: {}", filePath.u8string() );

        smp::EditTextFile( *this, filePath );
    }
    catch ( const fs::filesystem_error& e )
    {
        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, qwr::unicode::ToU8_FromAcpToWide( e.what() ) );
    }
    catch ( const qwr::QwrException& e )
    {
        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, e.what() );
    }
}

void CConfigTabPackage::OnEditScriptWith( UINT uNotifyCode, int nID, CWindow wndCtl )
{ // TODO: extract common code (see tab_script)
    switch ( nID )
    {
    case ID_EDIT_WITH_EXTERNAL:
    {
        // TODO: add ability to choose editor (e.g. like default context menu `Open With`)

        qwr::file::FileDialogOptions fdOpts{};
        fdOpts.filterSpec.assign( { { L"Executable files", L"*.exe" } } );
        fdOpts.defaultExtension = L"exe";

        try
        {
            const auto editorPathOpt = qwr::file::FileDialog( L"Choose text editor", false, fdOpts );
            if ( editorPathOpt )
            {
                const fs::path editorPath = *editorPathOpt;
                qwr::QwrException::ExpectTrue( fs::exists( editorPath ), "Invalid path" );

                smp::config::default_editor = editorPath.u8string();
            }
        }
        catch ( const fs::filesystem_error& e )
        {
            qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, qwr::unicode::ToU8_FromAcpToWide( e.what() ) );
        }
        break;
    }
    case ID_EDIT_WITH_INTERNAL:
    {
        config::default_editor = "";
        break;
    }
    default:
    {
        assert( false );
        break;
    }
    }

    OnEditScript( uNotifyCode, nID, wndCtl );
}

LONG CConfigTabPackage::OnEditScriptDropDown( LPNMHDR pnmh )
{
    const auto dropDown = reinterpret_cast<NMBCDROPDOWN*>( pnmh );

    POINT pt{ dropDown->rcButton.left, dropDown->rcButton.bottom };

    CWindow button = dropDown->hdr.hwndFrom;
    button.ClientToScreen( &pt );

    CMenu menu;
    if ( menu.CreatePopupMenu() )
    {
        menu.AppendMenu( MF_BYPOSITION, ID_EDIT_WITH_EXTERNAL, L"Edit with..." );
        menu.AppendMenu( MF_BYPOSITION, ID_EDIT_WITH_INTERNAL, L"Edit with internal editor" );
        menu.TrackPopupMenu( TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, m_hWnd, nullptr );
    }

    return 0;
}

void CConfigTabPackage::DoFullDdxToUi()
{
    if ( !this->m_hWnd )
    {
        return;
    }

    for ( auto& ddx: ddx_ )
    {
        ddx->WriteToUi();
    }

    UpdateUiButtons();
}

void CConfigTabPackage::UpdateUiButtons()
{
    const bool enableFileActions = !!focusedFileIdx_; ///< fileIdx == 0 <> main script file is selected
    CWindow{ GetDlgItem( IDC_BUTTON_REMOVE_FILE ) }.EnableWindow( enableFileActions );
    CWindow{ GetDlgItem( IDC_BUTTON_RENAME_FILE ) }.EnableWindow( enableFileActions );
}

void CConfigTabPackage::InitializeFilesListBox()
{
    try
    {
        filesListBox_ = GetDlgItem( IDC_LIST_PACKAGE_FILES );
        files_ = config::GetPackageScriptFiles( settings_ );
        UpdateListBoxFromData();
    }
    catch ( const fs::filesystem_error& e )
    {
        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, qwr::unicode::ToU8_FromAcpToWide( e.what() ) );
    }
}

void CConfigTabPackage::SortFiles()
{
    if ( files_.size() <= 1 )
    {
        return;
    }
    // skip first file (that is main file)
    std::sort( files_.begin() + 1, files_.end(), []( const auto& a, const auto& b ) {
        return ( StrCmpLogicalW( a.c_str(), b.c_str() ) < 0 );
    } );

    // move assets to the end
    const auto scriptDir = ( packagePath_ / "scripts" ).wstring();
    const auto it = std::find_if( files_.cbegin() + 1, files_.cend(), [&scriptDir]( const fs::path& a ) {
        return ( a.wstring().find( scriptDir ) == 0 );
    } );
    if ( it != files_.end() && std::distance( files_.cbegin(), it ) != 1 )
    {
        decltype( files_ ) sortedFiles;
        sortedFiles.reserve( files_.size() );

        sortedFiles.emplace_back( files_[0] );
        sortedFiles.insert( sortedFiles.end(), it, files_.cend() );
        sortedFiles.insert( sortedFiles.end(), files_.cbegin() + 1, it );

        files_ = sortedFiles;
    }
}

void CConfigTabPackage::UpdateListBoxFromData()
{
    try
    {
        SortFiles();

        const auto it = ranges::find( files_, focusedFile_ );
        assert( it != files_.cend() );

        focusedFileIdx_ = ranges::distance( files_.cbegin(), it );

        filesListBox_.ResetContent();
        for ( const auto& file: files_ )
        {
            filesListBox_.AddString( fs::relative( file, packagePath_ ).c_str() );
        }
    }
    catch ( const fs::filesystem_error& e )
    {
        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, qwr::unicode::ToU8_FromAcpToWide( e.what() ) );
    }
}

} // namespace smp::ui
