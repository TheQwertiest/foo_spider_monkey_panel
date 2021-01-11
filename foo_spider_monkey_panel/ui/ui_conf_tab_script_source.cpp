#include <stdafx.h>

#include "ui_conf_tab_script_source.h"

#include <fb2k/config.h>
#include <panel/edit_script.h>
#include <ui/ui_conf_new.h>

#include <qwr/error_popup.h>
#include <qwr/fb2k_paths.h>
#include <qwr/file_helpers.h>
#include <qwr/final_action.h>
#include <qwr/type_traits.h>
#include <qwr/winapi_error_helpers.h>

#include <filesystem>

namespace
{

using namespace smp;
using namespace smp::ui;

auto GetSampleFolderPath()
{
    return qwr::path::Component() / "samples";
}

std::vector<CConfigTabScriptSource::SampleComboBoxElem> GetSampleFileData()
{
    namespace fs = std::filesystem;

    std::vector<CConfigTabScriptSource::SampleComboBoxElem> elems;

    const auto sampleFolderPath = GetSampleFolderPath();

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

} // namespace

namespace smp::ui
{

std::vector<CConfigTabScriptSource::SampleComboBoxElem> CConfigTabScriptSource::sampleData_;

CConfigTabScriptSource::CConfigTabScriptSource( CDialogConfNew& parent, config::ParsedPanelSettings& settings )
    : parent_( parent )
    , settings_( settings )
    , ddx_( {
          qwr::ui::CreateUiDdx<qwr::ui::UiDdx_TextEdit>( path_, IDC_TEXTEDIT_SRC_PATH ),
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
        sampleData_ = GetSampleFileData();
    }
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
    InitializeLocalOptions();
    DoFullDdxToUi();
}

BOOL CConfigTabScriptSource::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    for ( auto& ddx: ddx_ )
    {
        ddx->SetHwnd( m_hWnd );
    }

    InitializeSamplesComboBox();
    InitializeLocalOptions();
    DoFullDdxToUi();

    return TRUE; // set focus to default control
}

void CConfigTabScriptSource::OnScriptSrcChange( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    if ( isInitializingUi_ )
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

    config::PanelSettings newSettings{};
    switch ( sourceTypeId_ )
    {
    case IDC_RADIO_SRC_SAMPLE:
    {
        newSettings.payload = ( sampleData_.empty()
                                    ? config::PanelSettings_Sample{}
                                    : config::PanelSettings_Sample{ qwr::unicode::ToU8( sampleData_[sampleIdx_].displayedName ) } );
        break;
    }
    case IDC_RADIO_SRC_FILE:
    {
        // TODO: add support for relative path (i.e. automatic truncation and expansion)
        // idea: truncate and expand path in config, not in UI
        newSettings.payload = config::PanelSettings_File{ path_ };
        break;
    }
    case IDC_RADIO_SRC_MEMORY:
    {
        newSettings.payload = config::PanelSettings_InMemory{};
        break;
    }
    case IDC_RADIO_SRC_PACKAGE:
    {
        // TODO: implement
        OnOpenPackageManager( 0, 0, nullptr );
        newSettings.payload = config::PanelSettings_InMemory{};
        break;
    }
    default:
    {
        assert( false );
        break;
    }
    }

    settings_ = config::ParsedPanelSettings::Parse( newSettings );

    DoRadioButtonsDdxToUi();
    parent_.OnScriptTypeChange();
}

void CConfigTabScriptSource::OnBrowseFile( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    if ( !parent_.IsCleanSlate() && !RequestConfirmationForReset() )
    {
        return;
    }

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
        return;
    }

    path_ = pathOpt->u8string();

    config::PanelSettings newSettings;
    newSettings.payload = config::PanelSettings_File{ path_ };

    settings_ = config::ParsedPanelSettings::Parse( newSettings );

    DoFullDdxToUi();
    DoRadioButtonsDdxToUi();
    parent_.OnScriptTypeChange();
}

void CConfigTabScriptSource::OnOpenPackageManager( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    //CDialogPackageManager pkgMgr("");
    //pkgMgr.DoModal();
}

void CConfigTabScriptSource::OnEditScript( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    namespace fs = std::filesystem;

    try
    {
        const auto hasChanged = panel::EditScript( *this, settings_ );
        if ( hasChanged )
        {
            parent_.OnDataChanged();
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
            qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, e.what() );
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

void CConfigTabScriptSource::InitializeLocalOptions()
{
    namespace fs = std::filesystem;

    path_ = ( settings_.scriptPath ? settings_.scriptPath->u8string() : std::u8string{} );

    sampleIdx_ = [&] {
        if ( settings_.GetSourceType() != config::ScriptSourceType::Sample )
        {
            return 0;
        }

        assert( settings_.scriptPath );

        const auto sampleName = fs::relative( *settings_.scriptPath, GetSampleFolderPath() ).wstring();
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
            settings_ = smp::config::ParsedPanelSettings::GetDefault();
            return 0;
        }

        assert( it != sampleData_.cend() );
        return ranges::distance( sampleData_.cbegin(), it );
    }();

    // Source is checked last, because it can be changed in the code above
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

    isInitializingUi_ = true;
    const qwr::final_action autoInit( [&] { isInitializingUi_ = false; } );

    for ( auto& ddx: ddx_ )
    {
        ddx->WriteToUi();
    }
    DoRadioButtonsDdxToUi();
}

void CConfigTabScriptSource::DoRadioButtonsDdxToUi()
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
    if ( sourceTypeId_ == IDC_RADIO_SRC_MEMORY )
    {
        assert( settings_.script );
        if ( settings_.script == config::PanelSettings_InMemory::GetDefaultScript() )
        {
            return true;
        }
        else
        {
            const int iRet = MessageBox(
                L"!!! Changing script type will reset all panel settings !!!\n"
                L"!!! Your whole script will be unrecoverably lost !!!\n\n"
                L"Are you sure?",
                L"Changing script type",
                MB_YESNO | MB_ICONWARNING );
            return ( iRet == IDYES );
        }
    }
    else if ( sourceTypeId_ == IDC_RADIO_SRC_PACKAGE )
    {
        const int ret = uMessageBox( m_hWnd, "Do you want to apply your changes?", "SMP package", MB_ICONWARNING | MB_SETFOREGROUND | MB_YESNOCANCEL );
        if ( ret == IDYES )
        {
            Apply();
        }

        return !( ret == IDCANCEL );
    }
    else
    {
        const int iRet = MessageBox(
            L"!!! Changing script type will reset all panel settings !!!\n\n"
            L"Are you sure?",
            L"Changing script type",
            MB_YESNO | MB_ICONWARNING );
        return ( iRet == IDYES );
    }
}

} // namespace smp::ui
