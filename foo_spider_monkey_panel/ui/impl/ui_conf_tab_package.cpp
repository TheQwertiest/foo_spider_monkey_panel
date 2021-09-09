#include <stdafx.h>

#include "ui_conf_tab_package.h"

#include <config/package_utils.h>
#include <fb2k/config.h>
#include <ui/ui_conf.h>
#include <ui/ui_input_box.h>
#include <utils/edit_text.h>
#include <utils/path_helpers.h>

#include <qwr/error_popup.h>
#include <qwr/file_helpers.h>
#include <qwr/final_action.h>
#include <qwr/ui_centered_message_box.h>
#include <qwr/winapi_error_helpers.h>

namespace fs = std::filesystem;

namespace
{

/// @throw qwr::QwrException
std::vector<fs::path> GetAllFilesFromPath( const fs::path& path )
{
    try
    {
        if ( fs::is_regular_file( path ) )
        {
            return { path };
        }
        else
        {
            return smp::utils::GetFilesRecursive( path );
        }
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

} // namespace

namespace smp::ui
{

CConfigTabPackage::CConfigTabPackage( CDialogConf& parent, config::ParsedPanelSettings& settings )
    : parent_( parent )
    , settings_( settings )
    , scriptName_( settings.scriptName )
    , scriptVersion_( settings.scriptVersion )
    , scriptAuthor_( settings.scriptAuthor )
    , scriptDescription_( settings.scriptDescription )
    , shouldGrabFocus_( settings.shouldGrabFocus )
    , enableDragDrop_( settings.enableDragDrop )
    , ddx_( { qwr::ui::CreateUiDdx<qwr::ui::UiDdx_TextEdit>( scriptName_, IDC_EDIT_PACKAGE_NAME ),
              qwr::ui::CreateUiDdx<qwr::ui::UiDdx_TextEdit>( scriptVersion_, IDC_EDIT_PACKAGE_VERSION ),
              qwr::ui::CreateUiDdx<qwr::ui::UiDdx_TextEdit>( scriptAuthor_, IDC_EDIT_PACKAGE_AUTHOR ),
              qwr::ui::CreateUiDdx<qwr::ui::UiDdx_TextEdit>( scriptDescription_, IDC_EDIT_PACKAGE_DESCRIPTION ),
              qwr::ui::CreateUiDdx<qwr::ui::UiDdx_CheckBox>( shouldGrabFocus_, IDC_CHECK_SHOULD_GRAB_FOCUS ),
              qwr::ui::CreateUiDdx<qwr::ui::UiDdx_CheckBox>( enableDragDrop_, IDC_CHECK_ENABLE_DRAG_N_DROP ),
              qwr::ui::CreateUiDdx<qwr::ui::UiDdx_ListBox>( focusedFileIdx_, IDC_LIST_PACKAGE_FILES ) } )
{
    InitializeLocalData();
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

void CConfigTabPackage::Refresh()
{
    if ( settings_.GetSourceType() == config::ScriptSourceType::Package
         && packagePath_ != config::GetPackagePath( settings_ ) )
    {
        InitializeLocalData();
    }
}

BOOL CConfigTabPackage::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    DlgResize_Init( false, true, WS_CHILD );

    for ( auto& ddx: ddx_ )
    {
        ddx->SetHwnd( m_hWnd );
    }

    InitializeFilesListBox();
    DoFullDdxToUi();

    suppressDdxFromUi_ = false;

    return TRUE; // set focus to default control
}

void CConfigTabPackage::OnDestroy()
{
    assert( pFilesListBoxDrop_ );
    pFilesListBoxDrop_->RevokeDragDrop();
    pFilesListBoxDrop_.Release();
}

void CConfigTabPackage::OnDdxUiChange( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    if ( suppressDdxFromUi_ )
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

    if ( nID != IDC_LIST_PACKAGE_FILES )
    {
        parent_.OnDataChanged();
    }
}

void CConfigTabPackage::OnNewScript( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    assert( static_cast<size_t>( focusedFileIdx_ ) < files_.size() );

    try
    {
        const auto scriptsDir = config::GetPackageScriptsDir( settings_ );

        const auto newFilenameOpt = [&]() -> std::optional<fs::path> {
            while ( true )
            {
                CInputBox dlg( "Enter script name", "Create new script file" );
                if ( dlg.DoModal( m_hWnd ) != IDOK )
                {
                    return std::nullopt;
                }

                auto path = scriptsDir / dlg.GetValue();
                if ( path.extension() != ".js" )
                {
                    path += ".js";
                }

                if ( fs::exists( path ) )
                {
                    qwr::ui::MessageBoxCentered(
                        *this,
                        L"File with this name already exists!",
                        L"Creating file",
                        MB_OK | MB_ICONWARNING );
                }
                else
                {
                    return path;
                }
            }
        }();

        if ( !newFilenameOpt )
        {
            return;
        }

        fs::create_directories( newFilenameOpt->parent_path() );
        qwr::file::WriteFile( *newFilenameOpt, "// empty" );

        files_.emplace_back( *newFilenameOpt );
        focusedFile_ = *newFilenameOpt;

        UpdateListBoxFromData();
        DoFullDdxToUi();
    }
    catch ( const fs::filesystem_error& e )
    {
        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, qwr::unicode::ToU8_FromAcpToWide( e.what() ) );
    }
}

void CConfigTabPackage::OnAddFile( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    qwr::file::FileDialogOptions fdOpts{};
    fdOpts.savePathGuid = guid::dialog_path;
    fdOpts.filterSpec.assign( {
        { L"All files", L"*.*" },
    } );

    const auto pathOpt = qwr::file::FileDialog( L"Add file", false, fdOpts );
    if ( !pathOpt )
    {
        return;
    }

    AddFile( *pathOpt );
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
        fs::remove_all( files_[focusedFileIdx_] );
    }
    catch ( const fs::filesystem_error& e )
    {
        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, qwr::unicode::ToU8_FromAcpToWide( e.what() ) );
    }

    files_.erase( files_.cbegin() + focusedFileIdx_ );
    focusedFile_ = files_[std::min<size_t>( focusedFileIdx_, files_.size() - 1 )];

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
            const int iRet = qwr::ui::MessageBoxCentered(
                *this,
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

        smp::EditTextFile( *this, filePath, true, true );
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

LRESULT CConfigTabPackage::OnScriptSaved( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    parent_.OnDataChanged();
    parent_.Apply();

    return 0;
}

LRESULT CConfigTabPackage::OnDropFiles( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    return pFilesListBoxDrop_->ProcessMessage(
        filesListBox_,
        wParam,
        lParam,
        [&]( const auto& path ) {
            AddFile( path );
        } );
}

void CConfigTabPackage::DoFullDdxToUi()
{
    if ( !this->m_hWnd )
    {
        return;
    }

    // avoid triggering loopback ddx
    suppressDdxFromUi_ = true;
    const qwr::final_action autoSuppress( [&] { suppressDdxFromUi_ = false; } );

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

void CConfigTabPackage::InitializeLocalData()
{
    packagePath_ = config::GetPackagePath( settings_ );
    isSample_ = settings_.isSample;
    mainScriptPath_ = *settings_.scriptPath;
    focusedFile_ = mainScriptPath_;
}

void CConfigTabPackage::InitializeFilesListBox()
{
    try
    {
        // !!! Important !!!
        // Groupbox *MUST* be *ABOVE* listbox in z-order for drag-n-drop on listbox to work
        // (makes no sense, but it doesn't work otherwise).
        // Check .rc and adjust if needed:
        // z-order in .rc is in reverse - last item is the top item.
        // !!! Important !!!

        filesListBox_ = GetDlgItem( IDC_LIST_PACKAGE_FILES );
        pFilesListBoxDrop_.Attach( new com::ComPtrImpl<com::FileDropTarget>( filesListBox_, *this ) );

        try
        {
            HRESULT hr = pFilesListBoxDrop_->RegisterDragDrop();
            qwr::error::CheckHR( hr, "RegisterDragDrop" );
        }
        catch ( qwr::QwrException& e )
        {
            qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, e.what() );
        }

        files_ = config::GetPackageFiles( settings_ );
        if ( const auto it = ranges::find( files_, focusedFile_ );
             it == files_.cend() )
        { // in case file was deleted
            focusedFile_ = mainScriptPath_;
        }

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
        return ( a < b );
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

void CConfigTabPackage::AddFile( const std::filesystem::path& path )
{
    try
    {
        const auto newPath = [&] {
            if ( path.extension() == ".js" )
            {
                return packagePath_ / "scripts" / path.filename();
            }
            else
            {
                return packagePath_ / "assets" / path.filename();
            }
        }();

        auto lastNewFile = focusedFile_;
        for ( const auto& file: GetAllFilesFromPath( path ) )
        {
            const auto newFile = ( file == path ? newPath : newPath / fs::relative( file, path ) );
            if ( fs::exists( newFile ) )
            {
                const int iRet = qwr::ui::MessageBoxCentered(
                    *this,
                    fmt::format( L"File already exists:\n"
                                 L"{}\n\n"
                                 L"Do you want to rewrite it?",
                                 newFile.wstring() )
                        .c_str(),
                    L"Adding file",
                    MB_YESNO | MB_ICONWARNING );
                if ( IDYES != iRet )
                {
                    continue;
                }
            }

            fs::create_directories( newFile.parent_path() );
            fs::copy( file, newFile, fs::copy_options::overwrite_existing );
            files_.emplace_back( newFile );
            lastNewFile = newFile;
        }

        focusedFile_ = lastNewFile;

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

} // namespace smp::ui
