#include <stdafx.h>

#include "ui_conf.h"

#include <config/package_utils.h>
#include <panel/js_panel_window.h>
#include <ui/ui_conf_tab_appearance.h>
#include <ui/ui_conf_tab_package.h>
#include <ui/ui_conf_tab_properties.h>
#include <ui/ui_conf_tab_script_source.h>
//#include <ui/ui_conf_tab_package_options.h>

#include <qwr/error_popup.h>

// TODO: fix default button highlighting and handling
// TODO: fix `TAB` button handling (cycle between controls)

namespace
{

bool IsCleanSlate( const smp::config::ParsedPanelSettings& settings )
{
    return ( settings.script == smp::config::PanelSettings_InMemory::GetDefaultScript() );
}

} // namespace

namespace
{

WINDOWPLACEMENT g_WindowPlacement{};

}

namespace smp::ui
{

CDialogConf::CDialogConf( smp::panel::js_panel_window* pParent, Tab tabId )
    : pParent_( pParent )
    , caption_( "Panel Configuration" )
    , oldSettings_( pParent->GetSettings() )
    , localSettings_( oldSettings_ )
    , oldProperties_( pParent->GetPanelProperties() )
    , localProperties_( oldProperties_ )
    , isCleanSlate_( ::IsCleanSlate( pParent->GetSettings() ) )
    , startingTabId_( tabId )
{
}

bool CDialogConf::IsCleanSlate() const
{
    return isCleanSlate_;
}

void CDialogConf::OnDataChanged()
{
    OnDataChangedImpl( true );
}

void CDialogConf::OnScriptTypeChange()
{
    bool tabLayoutChanged = ( oldSettings_.GetSourceType() != localSettings_.GetSourceType()
                              && ( oldSettings_.GetSourceType() == config::ScriptSourceType::Package
                                   || localSettings_.GetSourceType() == config::ScriptSourceType::Package ) );

    // package data is saved by the caller
    Apply( false );

    if ( tabLayoutChanged )
    {
        ReinitializeTabData();
        ReinitializeTabControls();
    }

    isCleanSlate_ = true;
}

bool CDialogConf::HasChanged()
{
    return ( hasChanged_
             || ranges::any_of( tabs_, []( const auto& pTab ) { return pTab->HasChanged(); } ) );
}

void CDialogConf::Apply( bool savePackageData )
{
    oldSettings_ = localSettings_;
    oldProperties_ = localProperties_;

    for ( auto& pTab: tabs_ )
    {
        pTab->Apply();
    }

    OnDataChangedImpl( false );

    if ( savePackageData )
    {
        try
        {
            config::MaybeSavePackageData( oldSettings_ );
        }
        catch ( const qwr::QwrException& e )
        {
            qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, e.what() );
        }
    }

    auto updatedSettings = oldSettings_.GeneratePanelSettings();
    updatedSettings.properties = oldProperties_;
    pParent_->UpdateSettings( updatedSettings );
}

void CDialogConf::Revert()
{
    localSettings_ = oldSettings_;
    localProperties_ = oldProperties_;

    for ( auto& pTab: tabs_ )
    {
        pTab->Revert();
    }

    OnDataChangedImpl( false );
}

void CDialogConf::SwitchTab( CDialogConf::Tab tabId )
{
    SetTabIdx( tabId );
    cTabs_.SetCurSel( activeTabIdx_ );
}

BOOL CDialogConf::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    cTabs_ = GetDlgItem( IDC_TAB_CONF );

    InitializeTabData( startingTabId_ );
    InitializeTabControls();

    CButton{ GetDlgItem( IDAPPLY ) }.EnableWindow( false );

    return TRUE; // set focus to default control
}

LRESULT CDialogConf::OnCloseCmd( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{
    { // Window position
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
        if ( HasChanged() )
        {
            const int ret = uMessageBox( m_hWnd, "Do you want to apply your changes?", caption_.c_str(), MB_ICONWARNING | MB_SETFOREGROUND | MB_YESNOCANCEL );
            switch ( ret )
            {
            case IDYES:
            {
                Apply();
                EndDialog( IDOK );
                break;
            }
            case IDCANCEL:
            {
                return 0;
            }
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

void CDialogConf::OnParentNotify( UINT message, UINT nChildID, LPARAM lParam )
{
    if ( WM_DESTROY == message && pcCurTab_ && reinterpret_cast<HWND>( lParam ) == static_cast<HWND>( *pcCurTab_ ) )
    {
        pcCurTab_ = nullptr;
    }
}

LRESULT CDialogConf::OnSelectionChanged( LPNMHDR pNmhdr )
{
    activeTabIdx_ = cTabs_.GetCurSel();
    CreateChildTab();

    return 0;
}

LRESULT CDialogConf::OnWindowPosChanged( UINT, WPARAM, LPARAM lp, BOOL& bHandled )
{
    auto lpwp = reinterpret_cast<LPWINDOWPOS>( lp );
    if ( lpwp->flags & SWP_HIDEWINDOW )
    {
        DestroyChildTab();
    }
    else if ( lpwp->flags & SWP_SHOWWINDOW && !pcCurTab_ )
    {
        CreateChildTab();
    }

    bHandled = FALSE;

    return 0;
}

void CDialogConf::OnDataChangedImpl( bool hasChanged )
{
    hasChanged_ = hasChanged;
    CButton{ GetDlgItem( IDAPPLY ) }.EnableWindow( hasChanged );
}

void CDialogConf::InitializeTabData( smp::ui::CDialogConf::Tab tabId )
{
    tabs_.clear();

    tabs_.emplace_back( std::make_unique<CConfigTabScriptSource>( *this, localSettings_ ) );
    if ( localSettings_.GetSourceType() == config::ScriptSourceType::Package )
    {
        tabs_.emplace_back( std::make_unique<CConfigTabPackage>( *this, localSettings_ ) );
    }
    tabs_.emplace_back( std::make_unique<CConfigTabAppearance>( *this, localSettings_ ) );
    tabs_.emplace_back( std::make_unique<CConfigTabProperties>( *this, localProperties_ ) );

    SetTabIdx( tabId );
}

void CDialogConf::ReinitializeTabData()
{
    tabs_.resize( 1 );

    if ( localSettings_.GetSourceType() == config::ScriptSourceType::Package )
    {
        tabs_.emplace_back( std::make_unique<CConfigTabPackage>( *this, localSettings_ ) );
    }
    tabs_.emplace_back( std::make_unique<CConfigTabAppearance>( *this, localSettings_ ) );
    tabs_.emplace_back( std::make_unique<CConfigTabProperties>( *this, localProperties_ ) );
}

void CDialogConf::InitializeTabControls()
{
    cTabs_.DeleteAllItems();

    for ( const auto& pTab: tabs_ )
    {
        cTabs_.AddItem( pTab->Name() );
    }

    cTabs_.SetCurSel( activeTabIdx_ );
    CreateChildTab();
}

void CDialogConf::ReinitializeTabControls()
{
    // do not recreate the first tab

    for ( auto&& i: ranges::views::ints( 1, cTabs_.GetItemCount() ) )
    {
        cTabs_.DeleteItem( 1 );
    }

    for ( const auto& pTab: tabs_ | ranges::views::drop( 1 ) )
    {
        cTabs_.AddItem( pTab->Name() );
    }
}

void CDialogConf::CreateChildTab()
{
    DestroyChildTab();

    RECT tabRc;

    cTabs_.GetWindowRect( &tabRc );
    ::MapWindowPoints( HWND_DESKTOP, m_hWnd, (LPPOINT)&tabRc, 2 );

    cTabs_.AdjustRect( FALSE, &tabRc );

    if ( activeTabIdx_ >= tabs_.size() )
    {
        activeTabIdx_ = 0;
    }

    auto& pCurTab = tabs_[activeTabIdx_];
    assert( pCurTab );
    pcCurTab_ = &pCurTab->Dialog();
    pCurTab->CreateTab( m_hWnd );

    EnableThemeDialogTexture( static_cast<HWND>( *pcCurTab_ ), ETDT_ENABLETAB );

    pcCurTab_->SetWindowPos( nullptr, tabRc.left, tabRc.top, tabRc.right - tabRc.left, tabRc.bottom - tabRc.top, SWP_NOZORDER );
    cTabs_.SetWindowPos( HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );

    pcCurTab_->ShowWindow( SW_SHOWNORMAL );
}

void CDialogConf::DestroyChildTab()
{
    if ( pcCurTab_ && static_cast<HWND>( *pcCurTab_ ) )
    {
        pcCurTab_->ShowWindow( SW_HIDE );
        pcCurTab_->DestroyWindow();
        pcCurTab_ = nullptr;
    }
}

void CDialogConf::SetTabIdx( CDialogConf::Tab tabId )
{
    switch ( tabId )
    {
    case Tab::script:
        activeTabIdx_ = 0;
        break;
    case Tab::package:
        activeTabIdx_ = 1;
        break;
    case Tab::properties:
        activeTabIdx_ = tabs_.size() - 1;
        break;
    default:
        assert( false );
        activeTabIdx_ = 0;
        break;
    }
}

} // namespace smp::ui
