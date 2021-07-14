#include <stdafx.h>

#include "ui_package_manager.h"

#include <config/package_utils.h>
#include <ui/ui_input_box.h>
#include <utils/zip_utils.h>

#include <component_paths.h>

#include <qwr/error_popup.h>
#include <qwr/fb2k_paths.h>
#include <qwr/file_helpers.h>
#include <qwr/final_action.h>
#include <qwr/ui_centered_message_box.h>
#include <qwr/winapi_error_helpers.h>

#include <filesystem>

namespace fs = std::filesystem;

namespace smp::ui
{

CDialogPackageManager::CDialogPackageManager( const qwr::u8string& currentPackageId )
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
    pPackagesListBoxDrop_.Attach( new com::ComPtrImpl<com::FileDropTarget>( packagesListBox_, *this ) );

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

void CDialogPackageManager::OnDdxUiChange( UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/ )
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

void CDialogPackageManager::OnNewPackage( UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/ )
{
    qwr::u8string curName;
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
            qwr::ui::MessageBoxCentered(
                *this,
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

void CDialogPackageManager::OnDeletePackage( UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/ )
{
    if ( packages_.empty() )
    {
        return;
    }

    assert( focusedPackageIdx_ >= 0 && static_cast<size_t>( focusedPackageIdx_ ) < packages_.size() );

    const int iRet = qwr::ui::MessageBoxCentered(
        *this, L"Are you sure you want to delete the package?", L"Deleting package", MB_YESNO );
    if ( iRet != IDYES )
    {
        return;
    }

    try
    {
        const auto packageId = packages_[focusedPackageIdx_].id;
        const auto packagePathOpt = config::FindPackage( packages_[focusedPackageIdx_].id );
        if ( packagePathOpt )
        {
            if ( config::IsPackageInUse( packageId ) )
            {
                config::MarkPackageAsToBeRemoved( packageId );

                auto it =
                    ranges::find_if( packages_,
                                     [&]( const auto& elem ) {
                                         return ( packageId == elem.id );
                                     } );
                if ( it != packages_.cend() )
                {
                    focusedPackageId_ = packageId;
                    it->status = config::PackageDelayStatus::ToBeRemoved;
                }

                if ( ConfirmRebootOnPackageInUse() )
                {
                    standard_commands::main_restart();
                }

                UpdateListBoxFromData();
                DoFullDdxToUi();
                return;
            }

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
    focusedPackageIdx_ = ( packages_.empty() ? -1 : std::max( 0, focusedPackageIdx_ - 1 ) );
    focusedPackageId_ = ( focusedPackageIdx_ == -1 ? qwr::u8string{} : packages_[focusedPackageIdx_].id );

    UpdateListBoxFromData();
    DoFullDdxToUi();
}

void CDialogPackageManager::OnImportPackage( UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/ )
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

    const auto isRestartNeeded = ImportPackage( *pathOpt );
    if ( isRestartNeeded && ConfirmRebootOnPackageInUse() )
    {
        standard_commands::main_restart();
    }
}

void CDialogPackageManager::OnExportPackage( UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/ )
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

void CDialogPackageManager::OnOpenFolder( UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/ )
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

LRESULT CDialogPackageManager::OnCloseCmd( WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/ )
{
    if ( wID != IDOK )
    {
        focusedPackageIdx_ = -1;
    }
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

LRESULT CDialogPackageManager::OnDropFiles( UINT /*uMsg*/, WPARAM wParam, LPARAM lParam )
{
    bool isRestartNeeded = false;
    const auto result = pPackagesListBoxDrop_->ProcessMessage(
        packagesListBox_,
        wParam,
        lParam,
        [&]( const auto& path ) { isRestartNeeded |= ImportPackage( path ); } );
    if ( isRestartNeeded && ConfirmRebootOnPackageInUse() )
    {
        standard_commands::main_restart();
    }

    return result;
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
    const auto isSample = ( currentPackageData.parsedSettings && currentPackageData.parsedSettings->isSample );

    CButton{ GetDlgItem( IDOK ) }.EnableWindow( !!currentPackageData.parsedSettings );
    CButton{ GetDlgItem( IDC_BUTTON_DELETE_PACKAGE ) }.EnableWindow( currentPackageData.status != config::PackageDelayStatus::ToBeRemoved && !isSample );
    CButton{ GetDlgItem( IDC_BUTTON_EXPORT_PACKAGE ) }.EnableWindow( !isSample );
    CButton{ GetDlgItem( IDC_BUTTON_OPEN_FOLDER ) }.EnableWindow( !!currentPackageData.parsedSettings );
}

void CDialogPackageManager::LoadPackages()
{
    packages_.clear();

    try
    {
        // TODO: consider extracting this code
        std::vector<fs::path> packagesDirs{ path::Packages_Profile(),
                                            path::Packages_Sample() };
        if ( qwr::path::Profile() != qwr::path::Foobar2000() )
        { // these paths might be the same when fb2k is in portable mode
            packagesDirs.emplace_back( path::Packages_Foobar2000() );
        }

        std::vector<qwr::u8string> packageIds;
        for ( const auto& packagesDir: packagesDirs )
        {
            if ( !fs::exists( packagesDir ) )
            {
                continue;
            }

            for ( const auto& dirIt: fs::directory_iterator( packagesDir ) )
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

            parsedPackages.back().status = config::GetPackageDelayStatus( packageId );
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
        const auto prefix = [&]() -> std::wstring {
            switch ( package.status )
            {
            case config::PackageDelayStatus::ToBeRemoved:
                return L"(will be removed) ";
            case config::PackageDelayStatus::ToBeUpdated:
                return L"(will be updated) ";
            default:
                return L"";
            }
        }();
        packagesListBox_.AddString( ( prefix + package.displayedName ).c_str() );
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
        const auto valueOrEmpty = []( const qwr::u8string& str ) -> std::wstring {
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
    const auto valueOrEmpty = []( const qwr::u8string& str ) -> qwr::u8string {
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
                        L"",
                        config::PackageDelayStatus::NotDelayed };
}

bool CDialogPackageManager::ImportPackage( const std::filesystem::path& path )
{
    assert( fs::exists( path ) );

    try
    {
        const auto tmpPath = path::TempFolder_PackageUnpack();
        fs::remove_all( tmpPath );
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

        UnpackZip( path, tmpPath );

        auto newSettings = config::GetPackageSettingsFromPath( tmpPath );
        const auto& packageId = *newSettings.packageId;

        if ( const auto oldPackagePathOpt = config::FindPackage( packageId );
             oldPackagePathOpt )
        {
            if ( !ConfirmPackageOverwrite( *oldPackagePathOpt, newSettings ) )
            {
                return false;
            }

            if ( config::IsPackageInUse( packageId ) )
            {
                config::MarkPackageAsToBeInstalled( packageId, tmpPath );

                auto it =
                    ranges::find_if( packages_,
                                     [&]( const auto& elem ) {
                                         return ( packageId == elem.id );
                                     } );
                if ( it != packages_.cend() )
                {
                    focusedPackageId_ = packageId;
                    it->status = config::PackageDelayStatus::ToBeUpdated;
                }

                UpdateListBoxFromData();
                DoFullDdxToUi();
                return true;
            }

            fs::remove_all( *oldPackagePathOpt );
        }

        const auto newPackagePath = path::Packages_Profile() / packageId;
        fs::create_directories( newPackagePath );
        fs::copy( tmpPath, newPackagePath, fs::copy_options::recursive );

        newSettings.scriptPath = newPackagePath / config::GetRelativePathToMainFile();

        auto it =
            ranges::find_if( packages_,
                             [&]( const auto& elem ) {
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
        focusedPackageId_ = packageId;

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

    return false;
}

bool CDialogPackageManager::ConfirmPackageOverwrite( const std::filesystem::path& oldPackagePath, const config::ParsedPanelSettings& newSettings )
{
    try
    {
        const auto oldSettings = config::GetPackageSettingsFromPath( oldPackagePath );

        const int iRet = qwr::ui::MessageBoxCentered(
            *this,
            qwr::unicode::ToWide(
                fmt::format( "Another version of this package is present:\n"
                             "old: '{}' vs new: '{}'\n\n"
                             "Do you want to update?",
                             oldSettings.scriptVersion.empty() ? "<none>" : oldSettings.scriptVersion,
                             newSettings.scriptVersion.empty() ? "<none>" : newSettings.scriptVersion ) )
                .c_str(),
            L"Importing package",
            MB_YESNO );
        if ( iRet != IDYES )
        {
            return false;
        }

        if ( oldSettings.scriptName != newSettings.scriptName )
        {
            const int iRet = qwr::ui::MessageBoxCentered(
                *this,
                qwr::unicode::ToWide(
                    fmt::format( "Currently installed package has a different name from the new one:\n"
                                 "old: '{}' vs new: '{}'\n\n"
                                 "Do you want to continue?",
                                 oldSettings.scriptName.empty() ? "<none>" : oldSettings.scriptName,
                                 newSettings.scriptName.empty() ? "<none>" : newSettings.scriptName ) )
                    .c_str(),
                L"Importing package",
                MB_YESNO | MB_ICONWARNING );
            if ( iRet != IDYES )
            {
                return false;
            }
        }
    }
    catch ( const qwr::QwrException& )
    {
        // old package might be broken and unparseable,
        // but we still need to confirm
        const int iRet = qwr::ui::MessageBoxCentered(
            *this,
            L"Another version of this package is present.\n"
            L"Do you want to update?",
            L"Importing package",
            MB_YESNO );
        if ( iRet != IDYES )
        {
            return false;
        }
    }

    return true;
}

bool CDialogPackageManager::ConfirmRebootOnPackageInUse()
{
    const int iRet = qwr::ui::MessageBoxCentered(
        *this,
        L"The package is currently in use. Changes will be applied on the next foobar2000 start.\n"
        L"Do you want to restart foobar2000 now?",
        L"Changing package",
        MB_YESNO );
    return ( iRet == IDYES );
}

} // namespace smp::ui
