#include <stdafx.h>

#include "ui_conf_tab_script_source.h"

#include <fb2k/config.h>
#include <panel/edit_script.h>
#include <ui/ui_conf.h>
#include <ui/ui_package_manager.h>

#include <component_paths.h>

#include <qwr/error_popup.h>
#include <qwr/fb2k_paths.h>
#include <qwr/file_helpers.h>
#include <qwr/final_action.h>
#include <qwr/type_traits.h>
#include <qwr/ui_centered_message_box.h>
#include <qwr/winapi_error_helpers.h>

#include <filesystem>

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

void UpdateOnSrcChange( config::ParsedPanelSettings& settings, const config::ParsedPanelSettings& newSettings )
{
    // panel id is persistent across src changes
    const auto oldPanelId = settings.panelId;
    settings = newSettings;
    settings.panelId = oldPanelId;
}

} // namespace

namespace smp::ui
{

std::vector<CConfigTabScriptSource::SampleComboBoxElem> CConfigTabScriptSource::sampleData_;

CConfigTabScriptSource::CConfigTabScriptSource( CDialogConf& parent, config::ParsedPanelSettings& settings )
    : parent_( parent )
    , settings_( settings )
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

    const auto newPayloadOpt = [&]() -> std::optional<config::PanelSettings::ScriptVariant> {
        switch ( sourceTypeId_ )
        {
        case IDC_RADIO_SRC_SAMPLE:
        {
            return ( sampleData_.empty()
                         ? config::PanelSettings_Sample{}
                         : config::PanelSettings_Sample{ qwr::unicode::ToU8( sampleData_[sampleIdx_].displayedName ) } );
        }
        case IDC_RADIO_SRC_MEMORY:
        {
            return config::PanelSettings_InMemory{};
        }
        case IDC_RADIO_SRC_FILE:
        {
            const auto parsedSettingsOpt = OnBrowseFileImpl();
            if ( parsedSettingsOpt )
            {
                path_ = parsedSettingsOpt->u8string();
                return config::PanelSettings_File{ path_ };
            }
            else
            {
                return std::nullopt;
            }
        }
        case IDC_RADIO_SRC_PACKAGE:
        {
            const auto parsedSettingsOpt = OnOpenPackageManagerImpl( settings_.packageId.value_or( "" ) );
            if ( parsedSettingsOpt )
            {
                packageName_ = parsedSettingsOpt->scriptName;
                return parsedSettingsOpt->GeneratePanelSettings().payload;
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

    if ( newPayloadOpt )
    {
        config::PanelSettings newSettings;
        newSettings.payload = *newPayloadOpt;
        try
        {
            UpdateOnSrcChange( settings_, config::ParsedPanelSettings::Parse( newSettings ) );

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

    config::PanelSettings newSettings;
    newSettings.payload = config::PanelSettings_File{ path_ };

    UpdateOnSrcChange( settings_, config::ParsedPanelSettings::Parse( newSettings ) );

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
    assert( settings_.packageId );

    const auto parsedSettingsOpt = OnOpenPackageManagerImpl( *settings_.packageId );
    if ( !parsedSettingsOpt )
    {
        return;
    }

    const auto isDifferentPackage = ( parsedSettingsOpt->packageId != settings_.packageId );

    if ( !parent_.IsCleanSlate()
         && isDifferentPackage
         && !RequestConfirmationOnPackageChange() )
    {
        return;
    }

    packageName_ = parsedSettingsOpt->scriptName;
    UpdateOnSrcChange( settings_, *parsedSettingsOpt );

    DoFullDdxToUi();
    DoButtonsDdxToUi();

    if ( isDifferentPackage )
    {
        parent_.OnWholeScriptChange();
    }
}

std::optional<config::ParsedPanelSettings>
CConfigTabScriptSource::OnOpenPackageManagerImpl( const qwr::u8string& packageId )
{
    CDialogPackageManager pkgMgr( packageId );
    pkgMgr.DoModal( m_hWnd );

    return pkgMgr.GetPackage();
}

void CConfigTabScriptSource::OnEditScript( UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/ )
{
    if ( settings_.GetSourceType() == config::ScriptSourceType::Package )
    {
        parent_.SwitchTab( CDialogConf::Tab::package );
    }
    else
    {
        try
        {
            panel::EditScript( *this, settings_ );
        }
        catch ( const qwr::QwrException& e )
        {
            qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, e.what() );
        }
    }
}

LONG CConfigTabScriptSource::OnEditScriptDropDown( LPNMHDR pnmh ) const
{
    auto const dropDown = reinterpret_cast<NMBCDROPDOWN*>( pnmh );

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

void CConfigTabScriptSource::OnEditScriptWith( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    namespace fs = std::filesystem;

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
        smp::config::default_editor = "";
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

LRESULT CConfigTabScriptSource::OnScriptSaved( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/ )
{
    parent_.OnDataChanged();
    parent_.Apply();

    return 0;
}

void CConfigTabScriptSource::InitializeLocalOptions()
{
    namespace fs = std::filesystem;

    path_ = ( settings_.scriptPath && settings_.GetSourceType() == config::ScriptSourceType::File
                  ? settings_.scriptPath->u8string()
                  : qwr::u8string{} );

    packageName_ = ( settings_.GetSourceType() == config::ScriptSourceType::Package
                         ? settings_.scriptName
                         : qwr::u8string{} );

    sampleIdx_ = [&] {
        if ( settings_.GetSourceType() != config::ScriptSourceType::Sample )
        {
            return 0;
        }

        assert( settings_.scriptPath );

        const auto sampleName = fs::relative( *settings_.scriptPath, path::ScriptSamples() ).wstring();
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
            UpdateOnSrcChange( settings_, smp::config::ParsedPanelSettings::GetDefault() );
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
        switch ( settings_.GetSourceType() )
        {
        case config::ScriptSourceType::Package:
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
        assert( settings_.script );
        if ( settings_.script == config::PanelSettings_InMemory::GetDefaultScript() )
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
    assert( settings_.packageId );
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
