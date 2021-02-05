#include <stdafx.h>

#include "ui_package_manager.h"

#include <config/package_utils.h>
#include <ui/ui_input_box.h>
#include <utils/array_x.h>
#include <utils/zip_utils.h>

#include <qwr/error_popup.h>
#include <qwr/fb2k_paths.h>
#include <qwr/file_helpers.h>
#include <qwr/final_action.h>
#include <qwr/winapi_error_helpers.h>

#include <filesystem>

namespace fs = std::filesystem;

// TODO: reimplement messagebox to enable centering on parent

namespace smp::ui
{

CDialogPackageManager::CDialogPackageManager( const std::u8string& currentPackageId )
    : focusedPackageId_( currentPackageId )
    , ddx_( {
          qwr::ui::CreateUiDdx<qwr::ui::UiDdx_ListBox>( focusedPackageIdx_, IDC_LIST_PACKAGES ),
      } )
{
}

std::optional<config::ParsedPanelSettings>
CDialogPackageManager::GetPackage() const
{
    if ( focusedPackageIdx_ < 0 )
    {
        return std::nullopt;
    }
    else
    {
        return packages_[focusedPackageIdx_].parsedSettings;
    }
}

LRESULT CDialogPackageManager::OnInitDialog( HWND, LPARAM )
{
    for ( auto& ddx: ddx_ )
    {
        ddx->SetHwnd( m_hWnd );
    }

    packagesListBox_ = GetDlgItem( IDC_LIST_PACKAGES );
    pPackagesListBoxDrop_.Attach( new com_object_impl_t<com::FileDropTarget>( packagesListBox_, *this ) );

    try
    {
        HRESULT hr = pPackagesListBoxDrop_->RegisterDragDrop();
        qwr::error::CheckHR( hr, "RegisterDragDrop" );
    }
    catch ( qwr::QwrException& e )
    {
        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, e.what() );
    }

    // TODO: add context menu
    packageInfoEdit_ = GetDlgItem( IDC_RICHEDIT_PACKAGE_INFO );
    packageInfoEdit_.SetWindowLong( GWL_EXSTYLE, packageInfoEdit_.GetWindowLong( GWL_EXSTYLE ) & ~WS_EX_CLIENTEDGE );
    packageInfoEdit_.SetEditStyle( SES_HYPERLINKTOOLTIPS | SES_NOFOCUSLINKNOTIFY, SES_HYPERLINKTOOLTIPS | SES_NOFOCUSLINKNOTIFY );
    packageInfoEdit_.SetAutoURLDetect();
    packageInfoEdit_.SetEventMask( packageInfoEdit_.GetEventMask() | ENM_LINK );

    SetWindowText( L"Script package manager" );

    CenterWindow();
    ::SetFocus( packagesListBox_ );

    LoadPackages();
    UpdateListBoxFromData();
    DoFullDdxToUi();

    return FALSE;
}

void CDialogPackageManager::OnDestroy()
{
    assert( pPackagesListBoxDrop_ );
    pPackagesListBoxDrop_->RevokeDragDrop();
    pPackagesListBoxDrop_.Release();
}

void CDialogPackageManager::OnDdxUiChange( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    auto it = ranges::find_if( ddx_, [nID]( auto& ddx ) {
        return ddx->IsMatchingId( nID );
    } );

    if ( ddx_.end() != it )
    {
        ( *it )->ReadFromUi();
    }

    if ( nID == IDC_LIST_PACKAGES )
    {
        UpdatedUiPackageInfo();
        UpdateUiButtons();
    }
}

void CDialogPackageManager::OnNewPackage( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    std::u8string curName;
    while ( true )
    {
        CInputBox dlg( "Enter new package name", "Creating new package", curName.c_str() );
        if ( dlg.DoModal( m_hWnd ) != IDOK )
        {
            return;
        }

        curName = dlg.GetValue();
        if ( curName.empty() )
        {
            MessageBox(
                L"Can't create package with empty name",
                L"Creating new package",
                MB_OK | MB_ICONWARNING );
            continue;
        }

        break;
    };

    try
    {
        const auto newSettings = config::GetNewPackageSettings( curName );
        config::MaybeSavePackageData( newSettings );

        packages_.emplace_back( GeneratePackageData( newSettings ) );
        focusedPackageId_ = *newSettings.packageId;

        UpdateListBoxFromData();
        DoFullDdxToUi();
    }
    catch ( const qwr::QwrException& e )
    {
        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, e.what() );
    }
}

void CDialogPackageManager::OnDeletePackage( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    if ( packages_.empty() )
    {
        return;
    }

    assert( focusedPackageIdx_ >= 0 && static_cast<size_t>( focusedPackageIdx_ ) < packages_.size() );

    const int iRet = MessageBox( L"Are you sure you want to delete the package?",
                                 L"Deleting package",
                                 MB_YESNO );
    if ( iRet != IDYES )
    {
        return;
    }

    try
    {
        const auto packagePathOpt = config::FindPackage( packages_[focusedPackageIdx_].id );
        if ( packagePathOpt )
        {
            fs::remove_all( *packagePathOpt );
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

    packages_.erase( packages_.cbegin() + focusedPackageIdx_ );
    focusedPackageIdx_ = ( packages_.size() ? std::max( 0, focusedPackageIdx_ - 1 ) : -1 );
    focusedPackageId_ = ( focusedPackageIdx_ == -1 ? std::u8string{} : packages_[focusedPackageIdx_].id );

    UpdateListBoxFromData();
    DoFullDdxToUi();
}

void CDialogPackageManager::OnImportPackage( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    qwr::file::FileDialogOptions fdOpts{};
    fdOpts.savePathGuid = guid::dialog_path;
    fdOpts.filterSpec.assign( {
        { L"Zip archives", L"*.zip" },
    } );

    const auto pathOpt = qwr::file::FileDialog( L"Import package", false, fdOpts );
    if ( !pathOpt )
    {
        return;
    }

    ImportPackage( *pathOpt );
}

void CDialogPackageManager::OnExportPackage( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    assert( focusedPackageIdx_ >= 0 && static_cast<size_t>( focusedPackageIdx_ ) < packages_.size() );
    assert( packages_[focusedPackageIdx_].parsedSettings );

    const auto& currentPackageData = packages_[focusedPackageIdx_];

    qwr::file::FileDialogOptions fdOpts{};
    fdOpts.savePathGuid = guid::dialog_path;
    fdOpts.filterSpec.assign( {
        { L"Zip archives", L"*.zip" },
    } );

    auto archiveName = currentPackageData.displayedName;
    for ( auto& ch: archiveName )
    {
        const auto ct = PathGetCharType( ch );
        if ( !( ct & GCT_LFNCHAR ) )
        {
            ch = L'_';
        }
    }

    fdOpts.defaultFilename = archiveName;
    fdOpts.defaultExtension = L"zip";

    const auto pathOpt = qwr::file::FileDialog( L"Save package as", true, fdOpts );
    if ( !pathOpt )
    {
        return;
    }

    try
    {
        ZipPacker zp{ fs::path( *pathOpt ) };
        zp.AddFolder( config::GetPackagePath( *currentPackageData.parsedSettings ) );
        zp.Finish();
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

void CDialogPackageManager::OnOpenFolder( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    assert( focusedPackageIdx_ >= 0 && static_cast<size_t>( focusedPackageIdx_ ) < packages_.size() );
    assert( packages_[focusedPackageIdx_].parsedSettings );

    try
    {
        const auto hInstance = ShellExecute( nullptr,
                                             L"explore",
                                             config::GetPackagePath( *packages_[focusedPackageIdx_].parsedSettings ).c_str(),
                                             nullptr,
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

LRESULT CDialogPackageManager::OnCloseCmd( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{
    EndDialog( wID );
    return 0;
}

LRESULT CDialogPackageManager::OnRichEditLinkClick( LPNMHDR pnmh )
{
    const auto* pEl = reinterpret_cast<ENLINK*>( pnmh );
    if ( pEl->msg == WM_LBUTTONUP )
    {
        std::wstring url;
        url.resize( pEl->chrg.cpMax - pEl->chrg.cpMin );
        packageInfoEdit_.GetTextRange( pEl->chrg.cpMin, pEl->chrg.cpMax, url.data() );

        ShellExecute( nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL );
    }

    return 0;
}

LRESULT CDialogPackageManager::OnDropFiles( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    auto pDataObj = reinterpret_cast<IDataObject*>( lParam );
    const auto autoDrop = qwr::final_action( [pDataObj] {
        pDataObj->Release();
    } );

    FORMATETC fmte = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stgm;
    if ( !SUCCEEDED( pDataObj->GetData( &fmte, &stgm ) ) )
    {
        return 0;
    }
    const auto autoStgm = qwr::final_action( [&stgm] {
        ReleaseStgMedium( &stgm );
    } );

    const auto hDrop = reinterpret_cast<HDROP>( stgm.hGlobal );

    const auto fileCount = DragQueryFile( hDrop, 0xFFFFFFFF, nullptr, 0 );
    if ( !fileCount )
    {
        return 0;
    }

    for ( const auto i: ranges::views::ints( 0, (int)fileCount ) )
    {
        const auto pathLength = DragQueryFile( hDrop, i, nullptr, 0 );
        std::wstring path;
        path.resize( pathLength + 1 );

        DragQueryFile( hDrop, i, path.data(), path.size() );
        path.resize( path.size() - 1 );

        ImportPackage( path );
    }

    return 0;
}

void CDialogPackageManager::DoFullDdxToUi()
{
    if ( !this->m_hWnd )
    {
        return;
    }

    for ( auto& ddx: ddx_ )
    {
        ddx->WriteToUi();
    }

    UpdatedUiPackageInfo();
    UpdateUiButtons();
}

void CDialogPackageManager::UpdateUiButtons()
{
    if ( focusedPackageIdx_ < 0 || static_cast<size_t>( focusedPackageIdx_ ) >= packages_.size() )
    {
        CButton{ GetDlgItem( IDOK ) }.EnableWindow( false );
        CButton{ GetDlgItem( IDC_BUTTON_DELETE_PACKAGE ) }.EnableWindow( false );
        CButton{ GetDlgItem( IDC_BUTTON_EXPORT_PACKAGE ) }.EnableWindow( false );
        CButton{ GetDlgItem( IDC_BUTTON_OPEN_FOLDER ) }.EnableWindow( false );
        return;
    }

    const auto& currentPackageData = packages_[focusedPackageIdx_];

    CButton{ GetDlgItem( IDOK ) }.EnableWindow( !!currentPackageData.parsedSettings );
    CButton{ GetDlgItem( IDC_BUTTON_DELETE_PACKAGE ) }.EnableWindow( currentPackageData.parsedSettings && !currentPackageData.parsedSettings->isSample );
    CButton{ GetDlgItem( IDC_BUTTON_EXPORT_PACKAGE ) }.EnableWindow( currentPackageData.parsedSettings && !currentPackageData.parsedSettings->isSample );
    CButton{ GetDlgItem( IDC_BUTTON_OPEN_FOLDER ) }.EnableWindow( !!currentPackageData.parsedSettings );
}

void CDialogPackageManager::LoadPackages()
{
    packages_.clear();

    try
    {
        // TODO: consider extracting this code
        std::vector<fs::path> packagesDirs{ qwr::path::Profile() / SMP_UNDERSCORE_NAME / "packages",
                                            qwr::path::Component() / "samples" / "packages" };
        if ( qwr::path::Profile() != qwr::path::Foobar2000() )
        { // these paths might be the same when fb2k is in portable mode
            packagesDirs.emplace_back( qwr::path::Foobar2000() / SMP_UNDERSCORE_NAME / "packages" );
        }

        std::vector<std::u8string> packageIds;
        for ( const auto& packagesDir: packagesDirs )
        {
            if ( !fs::exists( packagesDir ) )
            {
                continue;
            }

            for ( const auto dirIt: fs::directory_iterator( packagesDir ) )
            {
                const auto packageJson = dirIt.path() / L"package.json";
                if ( !fs::exists( packageJson ) || !fs::is_regular_file( packageJson ) )
                {
                    continue;
                }

                packageIds.emplace_back( dirIt.path().filename().u8string() );
            }
        }

        std::vector<PackageData> parsedPackages;
        for ( const auto& packageId: packageIds )
        {
            try
            {
                const auto packagePathOpt = config::FindPackage( packageId );
                qwr::QwrException::ExpectTrue( packagePathOpt.has_value(), "Could not find package with id: {}", packageId );

                const auto settings = config::GetPackageSettingsFromPath( *packagePathOpt );
                parsedPackages.emplace_back( GeneratePackageData( settings ) );
            }
            catch ( const qwr::QwrException& e )
            {
                PackageData packageData{ qwr::unicode::ToWide( fmt::format( "{} (ERROR)", packageId ) ),
                                         packageId,
                                         std::nullopt,
                                         qwr::unicode::ToWide( fmt::format( "Package parsing failed:\r\n{}", e.what() ) ) };
                parsedPackages.emplace_back( packageData );
            }
        }

        packages_ = std::move( parsedPackages );
    }
    catch ( const fs::filesystem_error& e )
    {
        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, qwr::unicode::ToU8_FromAcpToWide( e.what() ) );
    }
}

void CDialogPackageManager::SortPackages()
{
    ranges::sort( packages_,
                  []( const auto& a, const auto& b ) {
                      return ( StrCmpLogicalW( a.displayedName.c_str(), b.displayedName.c_str() ) < 0 );
                  } );
}

void CDialogPackageManager::UpdateListBoxFromData()
{
    SortPackages();

    const auto it = ranges::find_if( packages_, [&]( const auto& package ) { return ( focusedPackageId_ == package.id ); } );
    if ( it == packages_.cend() )
    {
        if ( packages_.empty() )
        {
            focusedPackageIdx_ = -1;
            focusedPackageId_.clear();
        }
        else
        {
            focusedPackageIdx_ = 0;
            focusedPackageId_ = packages_[0].id;
        }
    }
    else
    {
        focusedPackageIdx_ = ranges::distance( packages_.cbegin(), it );
    }

    packagesListBox_.ResetContent();
    for ( const auto& package: packages_ )
    {
        packagesListBox_.AddString( package.displayedName.c_str() );
    }
}

void CDialogPackageManager::UpdatedUiPackageInfo()
{
    if ( focusedPackageIdx_ < 0 )
    {
        packageInfoEdit_.SetWindowText( L"" );
        return;
    }

    const auto packageData = packages_[focusedPackageIdx_];

    packageInfoEdit_.SetWindowText( L"" );

    CHARFORMAT savedCharFormat{};
    packageInfoEdit_.GetSelectionCharFormat( savedCharFormat );
    const qwr::final_action autoFormat( [&] { packageInfoEdit_.SetSelectionCharFormat( savedCharFormat ); } );

    if ( !packageData.parsedSettings )
    {
        CHARFORMAT newCharFormat = savedCharFormat;
        newCharFormat.dwMask = CFM_COLOR;
        newCharFormat.dwEffects = ~CFE_AUTOCOLOR;
        newCharFormat.crTextColor = RGB( 255, 0, 0 );

        packageInfoEdit_.SetSelectionCharFormat( newCharFormat );
        packageInfoEdit_.AppendText( L"Error:\r\n", FALSE );

        packageInfoEdit_.SetSelectionCharFormat( savedCharFormat );
        packageInfoEdit_.AppendText( packages_[focusedPackageIdx_].errorText.c_str(), FALSE );
    }
    else
    {
        const auto valueOrEmpty = []( const std::u8string& str ) -> std::wstring {
            return ( str.empty() ? L"<empty>" : qwr::unicode::ToWide( str ) );
        };

        const auto& parsedSettings = *packageData.parsedSettings;

        CHARFORMAT newCharFormat = savedCharFormat;
        newCharFormat.dwMask = CFM_UNDERLINE;
        newCharFormat.dwEffects = CFM_UNDERLINE;

        const auto appendText = [&]( const wchar_t* field, const wchar_t* value ) {
            assert( field );
            assert( value );

            packageInfoEdit_.SetSelectionCharFormat( newCharFormat );
            packageInfoEdit_.AppendText( field, FALSE );
            packageInfoEdit_.SetSelectionCharFormat( savedCharFormat );
            packageInfoEdit_.AppendText( L": ", FALSE );
            packageInfoEdit_.AppendText( value, FALSE );
        };

        appendText( L"Name", valueOrEmpty( parsedSettings.scriptName ).c_str() );
        packageInfoEdit_.AppendText( L"\r\n", FALSE );

        appendText( L"Version", valueOrEmpty( parsedSettings.scriptVersion ).c_str() );
        packageInfoEdit_.AppendText( L"\r\n", FALSE );

        appendText( L"Author", valueOrEmpty( parsedSettings.scriptAuthor ).c_str() );
        packageInfoEdit_.AppendText( L"\r\n", FALSE );

        appendText( L"Description", ( L"\r\n" + valueOrEmpty( parsedSettings.scriptDescription ) ).c_str() );
    }
}

CDialogPackageManager::PackageData CDialogPackageManager::GeneratePackageData( const config::ParsedPanelSettings& parsedSettings )
{
    const auto displayedName = [&parsedSettings] {
        return ( parsedSettings.scriptAuthor.empty()
                     ? parsedSettings.scriptName
                     : fmt::format( "{} (by {})", parsedSettings.scriptName, parsedSettings.scriptAuthor ) );
    }();
    const auto valueOrEmpty = []( const std::u8string& str ) -> std::u8string {
        return ( str.empty() ? "<empty>" : str );
    };
    const auto displayedDescription = fmt::format( "Name: {}\r\n"
                                                   "Version: {}\r\n"
                                                   "Author: {}\r\n"
                                                   "Description:\r\n{}",
                                                   valueOrEmpty( parsedSettings.scriptName ),
                                                   valueOrEmpty( parsedSettings.scriptVersion ),
                                                   valueOrEmpty( parsedSettings.scriptAuthor ),
                                                   valueOrEmpty( parsedSettings.scriptDescription ) );

    return PackageData{ qwr::unicode::ToWide( displayedName ),
                        *parsedSettings.packageId,
                        parsedSettings,
                        L"" };
}

void CDialogPackageManager::ImportPackage( const std::filesystem::path& path )
{
    assert( fs::exists( path ) );

    try
    {
        const auto tmpPath = qwr::path::Profile() / SMP_UNDERSCORE_NAME / "tmp" / "unpacked_package";
        if ( fs::exists( tmpPath ) )
        {
            fs::remove_all( tmpPath );
        }
        fs::create_directories( tmpPath );
        qwr::final_action autoTmp( [&] {
            try
            {
                fs::remove_all( tmpPath );
            }
            catch ( const fs::filesystem_error& )
            {
            }
        } );

        UnpackZip( fs::path( path ), tmpPath );

        const auto newSettings = [&tmpPath] {
            auto settings = config::GetPackageSettingsFromPath( tmpPath );
            settings.scriptPath = qwr::path::Profile() / SMP_UNDERSCORE_NAME / "packages" / *settings.packageId / "main.js";
            return settings;
        }();

        if ( const auto oldPackagePathOpt = config::FindPackage( *newSettings.packageId );
             oldPackagePathOpt )
        {
            const int iRet = MessageBox(
                L"Another version of this package is present.\n"
                L"Do you want to update?",
                L"Importing package",
                MB_YESNO );
            if ( iRet != IDYES )
            {
                return;
            }

            try
            {
                const auto oldSettings = config::GetPackageSettingsFromPath( *oldPackagePathOpt );
                if ( oldSettings.scriptName != newSettings.scriptName )
                {
                    const int iRet = MessageBox(
                        qwr::unicode::ToWide(
                            fmt::format( "Currently installed package has a different name from the new one:\n"
                                         "old: '{}' vs new: '{}'\n\n"
                                         "Do you want to continue?",
                                         oldSettings.scriptName,
                                         newSettings.scriptName ) )
                            .c_str(),
                        L"Importing package",
                        MB_YESNO | MB_ICONWARNING );
                    if ( iRet != IDYES )
                    {
                        return;
                    }
                }
            }
            catch ( const qwr::QwrException& )
            {
            }

            fs::remove_all( *oldPackagePathOpt );
        }

        fs::copy( tmpPath, config::GetPackagePath( newSettings ), fs::copy_options::recursive );

        auto it =
            ranges::find_if( packages_,
                             [packageId = *newSettings.packageId]( const auto& elem ) {
                                 return ( packageId == elem.id );
                             } );
        if ( it != packages_.cend() )
        {
            *it = GeneratePackageData( newSettings );
        }
        else
        {
            packages_.emplace_back( GeneratePackageData( newSettings ) );
        }
        focusedPackageId_ = *newSettings.packageId;

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
