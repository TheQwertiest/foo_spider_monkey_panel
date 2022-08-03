#include <stdafx.h>

#include "ui_conf_tab_script_source.h"

#include <config/default_script.h>
#include <config/resolved_panel_script_settings.h>
#include <fb2k/config.h>
#include <panel/edit_script.h>
#include <ui/ui_conf.h>
#include <ui/ui_package_manager.h>
#include <utils/logging.h>

#include <component_paths.h>

#include <qwr/error_popup.h>
#include <qwr/fb2k_paths.h>
#include <qwr/file_helpers.h>
#include <qwr/final_action.h>
#include <qwr/type_traits.h>
#include <qwr/ui_centered_message_box.h>
#include <qwr/winapi_error_helpers.h>

#include <filesystem>

namespace fs = std::filesystem;

namespace
{

using namespace smp;
using namespace smp::ui;

/// @throw qwr::QwrException
std::vector<CConfigTabScriptSource::SampleComboBoxElem> GetSampleFileData()
{
    namespace fs = std::filesystem;

    try
    {
        std::vector<CConfigTabScriptSource::SampleComboBoxElem> elems;

        const auto sampleFolderPath = path::ScriptSamples();

        for ( const auto& subdir: { "complete", "jsplaylist-mod", "js-smooth", "basic" } )
        {
            for ( const auto& filepath: fs::directory_iterator( sampleFolderPath / subdir ) )
            {
                if ( filepath.path().extension() == ".js" )
                {
                    elems.emplace_back( filepath.path().wstring(), fs::relative( filepath, sampleFolderPath ) );
                }
            }
        }

        return elems;
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

} // namespace

namespace smp::ui
{

std::vector<CConfigTabScriptSource::SampleComboBoxElem> CConfigTabScriptSource::sampleData_;

CConfigTabScriptSource::CConfigTabScriptSource( CDialogConf& parent, config::PanelConfig& config )
    : parent_( parent )
    , config_( config )
    , ddx_( {
          qwr::ui::CreateUiDdx<qwr::ui::UiDdx_TextEdit>( path_, IDC_TEXTEDIT_SRC_PATH ),
          qwr::ui::CreateUiDdx<qwr::ui::UiDdx_TextEdit>( packageName_, IDC_TEXTEDIT_SRC_PACKAGE ),
          qwr::ui::CreateUiDdx<qwr::ui::UiDdx_ComboBox>( sampleIdx_, IDC_COMBO_SRC_SAMPLE ),
          qwr::ui::CreateUiDdx<qwr::ui::UiDdx_RadioRange>( sourceTypeId_,
                                                           std::initializer_list<int>{
                                                               IDC_RADIO_SRC_SAMPLE,
                                                               IDC_RADIO_SRC_MEMORY,
                                                               IDC_RADIO_SRC_FILE,
                                                               IDC_RADIO_SRC_PACKAGE,
                                                           } ),
      } )
{
    if ( sampleData_.empty() )
    { // can't initialize it during global initialization
        try
        {
            sampleData_ = GetSampleFileData();
        }
        catch ( const qwr::QwrException& e )
        {
            qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, e.what() );
        }
    }

    InitializeLocalOptions();
}

HWND CConfigTabScriptSource::CreateTab( HWND hParent )
{
    return Create( hParent );
}

CDialogImplBase& CConfigTabScriptSource::Dialog()
{
    return *this;
}

const wchar_t* CConfigTabScriptSource::Name() const
{
    return L"Script";
}

bool CConfigTabScriptSource::HasChanged()
{
    return false;
}

void CConfigTabScriptSource::Apply()
{
}

void CConfigTabScriptSource::Revert()
{
}

void CConfigTabScriptSource::Refresh()
{
}

BOOL CConfigTabScriptSource::OnInitDialog( HWND /*hwndFocus*/, LPARAM /*lParam*/ )
{
    DlgResize_Init( false, true, WS_CHILD );

    for ( auto& ddx: ddx_ )
    {
        ddx->SetHwnd( m_hWnd );
    }

    InitializeSamplesComboBox();
    DoFullDdxToUi();

    suppressUiDdx_ = false;

    return TRUE; // set focus to default control
}

void CConfigTabScriptSource::OnScriptSrcChange( UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/ )
{
    if ( suppressUiDdx_ )
    {
        return;
    }

    if ( !parent_.IsCleanSlate() && !RequestConfirmationForReset() )
    {
        DoFullDdxToUi();
        return;
    }

    auto it = ranges::find_if( ddx_, [nID]( auto& ddx ) { return ddx->IsMatchingId( nID ); } );
    if ( ddx_.end() != it )
    {
        ( *it )->ReadFromUi();
    }

    const auto newScriptSourceOpt = [&]() -> std::optional<config::RawScriptSourceVariant> {
        switch ( sourceTypeId_ )
        {
        case IDC_RADIO_SRC_SAMPLE:
        {
            return ( sampleData_.empty()
                         ? config::RawSampleFile{}
                         : config::RawSampleFile{ qwr::unicode::ToU8( sampleData_[sampleIdx_].displayedName ) } );
        }
        case IDC_RADIO_SRC_MEMORY:
        {
            return config::RawInMemoryScript{ config::GetDefaultScript() };
        }
        case IDC_RADIO_SRC_FILE:
        {
            const auto scriptFileOpt = OnBrowseFileImpl();
            if ( scriptFileOpt )
            {
                path_ = scriptFileOpt->u8string();
                return config::RawScriptFile{ path_ };
            }
            else
            {
                return std::nullopt;
            }
        }
        case IDC_RADIO_SRC_PACKAGE:
        {
            const auto packageId = ( config::GetSourceType( config_.scriptSource ) == config::ScriptSourceType::SmpPackage ? std::get<config::RawSmpPackage>( config_.scriptSource ).id : "" );
            const auto rawPackageOpt = OnOpenPackageManagerImpl( packageId );
            if ( rawPackageOpt )
            {
                packageName_ = rawPackageOpt->name;
                return rawPackageOpt;
            }
            else
            {
                return std::nullopt;
            }
        }
        default:
        {
            assert( false );
            return std::nullopt;
        }
        }
    }();

    if ( newScriptSourceOpt )
    {
        config_.scriptSource = *newScriptSourceOpt;
        try
        {
            DoButtonsDdxToUi();
            DoFullDdxToUi();
            parent_.OnWholeScriptChange();

            return;
        }
        catch ( const qwr::QwrException& e )
        {
            qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, e.what() );
        }
    }

    InitializeSourceType();
    OnDdxValueChange( sourceTypeId_ );
}

void CConfigTabScriptSource::OnDdxValueChange( int nID )
{
    // avoid triggering loopback ddx
    suppressUiDdx_ = true;
    const qwr::final_action autoSuppress( [&] { suppressUiDdx_ = false; } );

    auto it = std::find_if( ddx_.begin(), ddx_.end(), [nID]( auto& val ) {
        return val->IsMatchingId( nID );
    } );

    assert( ddx_.end() != it );
    ( *it )->WriteToUi();
}

void CConfigTabScriptSource::OnBrowseFile( UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/ )
{
    if ( !parent_.IsCleanSlate() && !RequestConfirmationForReset() )
    {
        return;
    }

    const auto pathOpt = OnBrowseFileImpl();
    if ( !pathOpt || pathOpt->empty() )
    {
        return;
    }

    path_ = pathOpt->u8string();
    config_.scriptSource = config::RawScriptFile{ path_ };

    DoFullDdxToUi();
    DoButtonsDdxToUi();
    parent_.OnWholeScriptChange();
}

std::optional<std::filesystem::path> CConfigTabScriptSource::OnBrowseFileImpl()
{
    qwr::file::FileDialogOptions fdOpts{};
    fdOpts.savePathGuid = guid::dialog_path;
    fdOpts.filterSpec.assign( {
        { L"JavaScript files", L"*.js" },
        { L"Text files", L"*.txt" },
        { L"All files", L"*.*" },
    } );
    fdOpts.defaultExtension = L"js";

    const auto pathOpt = qwr::file::FileDialog( L"Open script file", false, fdOpts );
    if ( !pathOpt || pathOpt->empty() )
    {
        return std::nullopt;
    }

    return pathOpt;
}

void CConfigTabScriptSource::OnOpenPackageManager( UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/ )
{
    assert( config::GetSourceType( config_.scriptSource ) == config::ScriptSourceType::SmpPackage );
    const auto& rawPackage = std::get<config::RawSmpPackage>( config_.scriptSource );

    const auto newRawPackageOpt = OnOpenPackageManagerImpl( rawPackage.id );
    if ( !newRawPackageOpt )
    {
        return;
    }

    const auto isDifferentPackage = ( newRawPackageOpt->id != rawPackage.id );

    if ( !parent_.IsCleanSlate()
         && isDifferentPackage
         && !RequestConfirmationOnPackageChange() )
    {
        return;
    }

    packageName_ = newRawPackageOpt->name;
    config_.scriptSource = *newRawPackageOpt;

    DoFullDdxToUi();
    DoButtonsDdxToUi();

    if ( isDifferentPackage )
    {
        parent_.OnWholeScriptChange();
    }
}

std::optional<config::RawSmpPackage>
CConfigTabScriptSource::OnOpenPackageManagerImpl( const qwr::u8string& packageId )
{
    CDialogPackageManager pkgMgr( packageId );
    pkgMgr.DoModal( m_hWnd );

    return pkgMgr.GetPackage();
}

void CConfigTabScriptSource::OnEditScript( UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/ )
{
    try
    {
        switch ( config::GetSourceType( config_.scriptSource ) )
        {
        case config::ScriptSourceType::InMemory:
        {
            panel::EditScript( *this, std::get<config::RawInMemoryScript>( config_.scriptSource ).script );
            break;
        }
        case config::ScriptSourceType::File:
        case config::ScriptSourceType::Sample:
        {
            panel::EditScriptFile( *this, config::ResolvedPanelScriptSettings::ResolveSource( config_.scriptSource ) );
            break;
        }
        case config::ScriptSourceType::SmpPackage:
        {
            parent_.SwitchTab( CDialogConf::Tab::package );
            break;
        }
        default:
        {
            assert( false );
            break;
        }
        }
    }
    catch ( const qwr::QwrException& e )
    {
        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, e.what() );
    }
}

LONG CConfigTabScriptSource::OnEditScriptDropDown( LPNMHDR pnmh ) const
{
    auto const dropDown = reinterpret_cast<NMBCDROPDOWN*>( pnmh );

    POINT pt{ dropDown->rcButton.left, dropDown->rcButton.bottom };

    CWindow button = dropDown->hdr.hwndFrom;
    button.ClientToScreen( &pt );

    CMenuEditWith menu;
    if ( !menu.CreatePopupMenu() )
    {
        return 0;
    }

    menu.InitMenu();
    menu.TrackPopupMenu( TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, m_hWnd, nullptr );

    return 0;
}

void CConfigTabScriptSource::OnEditScriptDropDownClick( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    OnEditScript( uNotifyCode, nID, wndCtl );
}

LRESULT CConfigTabScriptSource::OnScriptSaved( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/ )
{
    parent_.OnDataChanged();
    parent_.Apply();

    return 0;
}

void CConfigTabScriptSource::InitializeLocalOptions()
{
    const auto sourceType = config::GetSourceType( config_.scriptSource );

    path_ = ( sourceType == config::ScriptSourceType::File
                  ? std::get<config::RawScriptFile>( config_.scriptSource ).scriptPath.u8string()
                  : qwr::u8string{} );

    packageName_ = ( sourceType == config::ScriptSourceType::SmpPackage
                         ? std::get<config::RawSmpPackage>( config_.scriptSource ).name
                         : qwr::u8string{} );

    sampleIdx_ = [&] {
        if ( sourceType != config::ScriptSourceType::Sample )
        {
            return 0;
        }

        const auto sampleName = qwr::unicode::ToWide( std::get<config::RawSampleFile>( config_.scriptSource ).name );
        if ( sampleName.empty() )
        {
            return 0;
        }

        const auto it = ranges::find_if( sampleData_, [&sampleName]( const auto& elem ) {
            return ( sampleName == elem.displayedName );
        } );

        if ( it == sampleData_.cend() )
        {
            qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, fmt::format( "Can't find sample `{}`. Your settings will be reset.", qwr::unicode::ToU8( sampleName ) ) );
            config_.scriptSource = config::RawSampleFile{ config::GetDefaultScript() };
            return 0;
        }

        assert( it != sampleData_.cend() );
        return ranges::distance( sampleData_.cbegin(), it );
    }();

    // Source is checked last, because it can be changed in the code above
    InitializeSourceType();
}

void CConfigTabScriptSource::InitializeSourceType()
{
    sourceTypeId_ = [&] {
        switch ( config::GetSourceType( config_.scriptSource ) )
        {
        case config::ScriptSourceType::SmpPackage:
            return IDC_RADIO_SRC_PACKAGE;
        case config::ScriptSourceType::Sample:
            return IDC_RADIO_SRC_SAMPLE;
        case config::ScriptSourceType::File:
            return IDC_RADIO_SRC_FILE;
        case config::ScriptSourceType::InMemory:
            return IDC_RADIO_SRC_MEMORY;
        default:
            assert( false );
            return IDC_RADIO_SRC_MEMORY;
        }
    }();
}

void CConfigTabScriptSource::DoFullDdxToUi()
{
    if ( !this->m_hWnd )
    {
        return;
    }

    // avoid triggering loopback ddx
    suppressUiDdx_ = true;
    const qwr::final_action autoSuppress( [&] { suppressUiDdx_ = false; } );

    for ( auto& ddx: ddx_ )
    {
        ddx->WriteToUi();
    }
    DoButtonsDdxToUi();
}

void CConfigTabScriptSource::DoSourceTypeDdxToUi()
{
    if ( !this->m_hWnd )
    {
        return;
    }

    switch ( sourceTypeId_ )
    {
    case IDC_RADIO_SRC_SAMPLE:
    {
        CWindow{ GetDlgItem( IDC_COMBO_SRC_SAMPLE ) }.EnableWindow( true );
        CWindow{ GetDlgItem( IDC_TEXTEDIT_SRC_PATH ) }.EnableWindow( false );
        CWindow{ GetDlgItem( IDC_BUTTON_BROWSE ) }.EnableWindow( false );
        CWindow{ GetDlgItem( IDC_BUTTON_OPEN_PKG_MGR ) }.EnableWindow( false );
        break;
    }
    case IDC_RADIO_SRC_MEMORY:
    {
        CWindow{ GetDlgItem( IDC_COMBO_SRC_SAMPLE ) }.EnableWindow( false );
        CWindow{ GetDlgItem( IDC_TEXTEDIT_SRC_PATH ) }.EnableWindow( false );
        CWindow{ GetDlgItem( IDC_BUTTON_BROWSE ) }.EnableWindow( false );
        CWindow{ GetDlgItem( IDC_BUTTON_OPEN_PKG_MGR ) }.EnableWindow( false );
        break;
    }
    case IDC_RADIO_SRC_FILE:
    {
        CWindow{ GetDlgItem( IDC_COMBO_SRC_SAMPLE ) }.EnableWindow( false );
        CWindow{ GetDlgItem( IDC_TEXTEDIT_SRC_PATH ) }.EnableWindow( true );
        CWindow{ GetDlgItem( IDC_BUTTON_BROWSE ) }.EnableWindow( true );
        CWindow{ GetDlgItem( IDC_BUTTON_OPEN_PKG_MGR ) }.EnableWindow( false );
        break;
    }
    case IDC_RADIO_SRC_PACKAGE:
    {
        CWindow{ GetDlgItem( IDC_COMBO_SRC_SAMPLE ) }.EnableWindow( false );
        CWindow{ GetDlgItem( IDC_TEXTEDIT_SRC_PATH ) }.EnableWindow( false );
        CWindow{ GetDlgItem( IDC_BUTTON_BROWSE ) }.EnableWindow( false );
        CWindow{ GetDlgItem( IDC_BUTTON_OPEN_PKG_MGR ) }.EnableWindow( true );
        break;
    }
    default:
    {
        assert( false );
        break;
    }
    }
}

void CConfigTabScriptSource::DoButtonsDdxToUi()
{
    if ( !this->m_hWnd )
    {
        return;
    }

    switch ( sourceTypeId_ )
    {
    case IDC_RADIO_SRC_SAMPLE:
    {
        CWindow{ GetDlgItem( IDC_COMBO_SRC_SAMPLE ) }.EnableWindow( true );
        CWindow{ GetDlgItem( IDC_TEXTEDIT_SRC_PATH ) }.EnableWindow( false );
        CWindow{ GetDlgItem( IDC_BUTTON_BROWSE ) }.EnableWindow( false );
        CWindow{ GetDlgItem( IDC_TEXTEDIT_SRC_PACKAGE ) }.EnableWindow( false );
        CWindow{ GetDlgItem( IDC_BUTTON_OPEN_PKG_MGR ) }.EnableWindow( false );
        break;
    }
    case IDC_RADIO_SRC_MEMORY:
    {
        CWindow{ GetDlgItem( IDC_COMBO_SRC_SAMPLE ) }.EnableWindow( false );
        CWindow{ GetDlgItem( IDC_TEXTEDIT_SRC_PATH ) }.EnableWindow( false );
        CWindow{ GetDlgItem( IDC_BUTTON_BROWSE ) }.EnableWindow( false );
        CWindow{ GetDlgItem( IDC_TEXTEDIT_SRC_PACKAGE ) }.EnableWindow( false );
        CWindow{ GetDlgItem( IDC_BUTTON_OPEN_PKG_MGR ) }.EnableWindow( false );
        break;
    }
    case IDC_RADIO_SRC_FILE:
    {
        CWindow{ GetDlgItem( IDC_COMBO_SRC_SAMPLE ) }.EnableWindow( false );
        CWindow{ GetDlgItem( IDC_TEXTEDIT_SRC_PATH ) }.EnableWindow( true );
        CWindow{ GetDlgItem( IDC_BUTTON_BROWSE ) }.EnableWindow( true );
        CWindow{ GetDlgItem( IDC_TEXTEDIT_SRC_PACKAGE ) }.EnableWindow( false );
        CWindow{ GetDlgItem( IDC_BUTTON_OPEN_PKG_MGR ) }.EnableWindow( false );
        break;
    }
    case IDC_RADIO_SRC_PACKAGE:
    {
        CWindow{ GetDlgItem( IDC_COMBO_SRC_SAMPLE ) }.EnableWindow( false );
        CWindow{ GetDlgItem( IDC_TEXTEDIT_SRC_PATH ) }.EnableWindow( false );
        CWindow{ GetDlgItem( IDC_BUTTON_BROWSE ) }.EnableWindow( false );
        CWindow{ GetDlgItem( IDC_TEXTEDIT_SRC_PACKAGE ) }.EnableWindow( true );
        CWindow{ GetDlgItem( IDC_BUTTON_OPEN_PKG_MGR ) }.EnableWindow( true );
        break;
    }
    default:
    {
        assert( false );
        break;
    }
    }
}

void CConfigTabScriptSource::InitializeSamplesComboBox()
{
    samplesComboBox_ = GetDlgItem( IDC_COMBO_SRC_SAMPLE );
    for ( const auto& elem: sampleData_ )
    {
        samplesComboBox_.AddString( elem.displayedName.c_str() );
    }
}

bool CConfigTabScriptSource::RequestConfirmationForReset()
{
    // TODO: fix revert (or move the confirmation to parent window)
    if ( sourceTypeId_ == IDC_RADIO_SRC_MEMORY )
    {
        assert( config::GetSourceType( config_.scriptSource ) == config::ScriptSourceType::InMemory );
        if ( std::get<config::RawInMemoryScript>( config_.scriptSource ).script == config::GetDefaultScript() )
        {
            return true;
        }
        else
        {
            const int iRet = qwr::ui::MessageBoxCentered(
                *this,
                L"!!! Changing script type will reset all panel settings !!!\n"
                L"!!! Your whole script will be unrecoverably lost !!!\n\n"
                L"Are you sure?",
                L"Changing script type",
                MB_YESNO | MB_ICONWARNING );
            return ( IDYES == iRet );
        }
    }
    else
    {
        const int iRet = qwr::ui::MessageBoxCentered(
            *this,
            L"!!! Changing script type will reset all panel settings !!!\n\n"
            L"Are you sure?",
            L"Changing script type",
            MB_YESNO | MB_ICONWARNING );
        if ( iRet != IDYES )
        {
            return false;
        }

        if ( sourceTypeId_ == IDC_RADIO_SRC_PACKAGE && parent_.HasChanged() )
        {
            const int iRet = qwr::ui::MessageBoxCentered(
                *this,
                L"Do you want to save your changes to package?",
                TEXT( SMP_NAME ),
                MB_ICONWARNING | MB_SETFOREGROUND | MB_YESNOCANCEL );
            switch ( iRet )
            {
            case IDYES:
                parent_.Apply();
                break;
            case IDNO:
                parent_.Revert();
                break;
            case IDCANCEL:
            default:
                return false;
            }
        }

        return true;
    }
}

bool CConfigTabScriptSource::RequestConfirmationOnPackageChange()
{
    assert( config::GetSourceType( config_.scriptSource ) == config::ScriptSourceType::SmpPackage );
    assert( sourceTypeId_ == IDC_RADIO_SRC_PACKAGE );

    const int iRet = qwr::ui::MessageBoxCentered(
        *this,
        L"!!! Changing package will reset all panel settings !!!\n\n"
        L"Are you sure?",
        L"Changing script type",
        MB_YESNO | MB_ICONWARNING );
    if ( iRet != IDYES )
    {
        return false;
    }

    if ( parent_.HasChanged() )
    {
        const int iRet = qwr::ui::MessageBoxCentered( m_hWnd, L"Do you want to save your changes to package?", TEXT( SMP_NAME ), MB_ICONWARNING | MB_SETFOREGROUND | MB_YESNOCANCEL );
        switch ( iRet )
        {
        case IDYES:
            parent_.Apply();
            break;
        case IDNO:
            parent_.Revert();
            break;
        case IDCANCEL:
        default:
            return false;
        }
    }

    return true;
}

} // namespace smp::ui
