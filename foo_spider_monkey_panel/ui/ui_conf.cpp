#include <stdafx.h>

#include "ui_conf.h"

#include <panel/panel_window.h>
#include <ui/impl/ui_conf_tab_appearance.h>
#include <ui/impl/ui_conf_tab_package.h>
#include <ui/impl/ui_conf_tab_properties.h>
#include <ui/impl/ui_conf_tab_script_source.h>
#include <utils/guid_helpers.h>

#include <component_paths.h>

#include <qwr/error_popup.h>
#include <qwr/fb2k_paths.h>
#include <qwr/string_helpers.h>
#include <qwr/ui_centered_message_box.h>

namespace
{

bool IsCleanSlate( const smp::config::PanelConfig& config )
{
    const smp::config::PanelConfig defaultConfig;
    return ( config.scriptSource.index() == defaultConfig.scriptSource.index()
             && std::get<smp::config::RawInMemoryScript>( config.scriptSource ).script == std::get<smp::config::RawInMemoryScript>( defaultConfig.scriptSource ).script );
}

} // namespace

namespace
{

WINDOWPLACEMENT g_WindowPlacement{};

}

namespace smp::ui
{

CDialogConf::CDialogConf( smp::panel::PanelWindow* pParent, Tab tabId )
    : pParent_( pParent )
    , isCleanSlate_( ::IsCleanSlate( pParent->GetPanelConfig() ) )
    , panelNameDdx_(
          qwr::ui::CreateUiDdx<qwr::ui::UiDdx_TextEdit>( localConfig_.panelSettings.id, IDC_EDIT_PANEL_NAME ) )
    , startingTabId_( tabId )
{
    InitializeLocalData();
}

bool CDialogConf::IsCleanSlate() const
{
    return isCleanSlate_;
}

void CDialogConf::OnDataChanged()
{
    OnDataChangedImpl( true );
}

void CDialogConf::OnWholeScriptChange()
{
    const auto tabLayoutChanged = ( config::GetSourceType( oldConfig_.scriptSource ) != config::GetSourceType( localConfig_.scriptSource )
                                    && ( config::GetSourceType( oldConfig_.scriptSource ) == config::ScriptSourceType::SmpPackage
                                         || config::GetSourceType( localConfig_.scriptSource ) == config::ScriptSourceType::SmpPackage ) );

    OnDataChangedImpl( true );

    localConfig_.properties.values.clear();
    // package data is saved by the caller
    Apply();

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

void CDialogConf::Apply()
{
    if ( !hasChanged_ )
    {
        return;
    }

    oldConfig_ = localConfig_;

    for ( auto& pTab: tabs_ )
    {
        pTab->Apply();
    }

    OnDataChangedImpl( false );
    DisablePanelNameControls();

    pParent_->UpdateSettings( oldConfig_ );

    // setting might've been modified by the script
    InitializeLocalData();

    suppressDdxFromUi_ = true;
    const auto ddxSuppress = qwr::final_action( [&] { suppressDdxFromUi_ = false; } );
    DoFullDdxToUi();
    RefreshTabData();
}

void CDialogConf::Revert()
{
    localConfig_ = oldConfig_;

    for ( auto& pTab: tabs_ )
    {
        pTab->Revert();
    }

    OnDataChangedImpl( false );
}

void CDialogConf::SwitchTab( CDialogConf::Tab tabId )
{
    SetActiveTabIdx( tabId );

    if ( !MaybeInitializeSmpPackageTab() )
    {
        activeTabIdx_ = GetTabIdx( Tab::script );
    }
    cTabs_.SetCurSel( activeTabIdx_ );
    CreateChildTab();
}

BOOL CDialogConf::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    DlgResize_Init( false );

    cTabs_ = GetDlgItem( IDC_TAB_CONF );

    InitializeTabData( startingTabId_ );
    InitializeTabControls();

    DisablePanelNameControls();
    if ( pParent_->IsPanelIdOverridenByScript() )
    {
        CButton{ GetDlgItem( IDC_BUTTON_EDIT_PANEL_NAME ) }.EnableWindow( false );
    }

    CButton{ GetDlgItem( IDAPPLY ) }.EnableWindow( false );

    panelNameDdx_->SetHwnd( m_hWnd );

    DoFullDdxToUi();

    suppressDdxFromUi_ = false;

    return TRUE; // set focus to default control
}

LRESULT CDialogConf::OnSize( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    CDialogResize<CDialogConf>::OnSize( uMsg, wParam, lParam, bHandled );

    if ( !cTabs_ || !pcCurTab_ )
    {
        bHandled = TRUE;
        return 0;
    }

    RECT tabRc;

    cTabs_.GetWindowRect( &tabRc );
    ::MapWindowPoints( HWND_DESKTOP, m_hWnd, (LPPOINT)&tabRc, 2 );

    cTabs_.AdjustRect( FALSE, &tabRc );

    pcCurTab_->SetWindowPos( nullptr, tabRc.left, tabRc.top, tabRc.right - tabRc.left, tabRc.bottom - tabRc.top, SWP_NOZORDER );

    bHandled = TRUE;
    return 0;
}

void CDialogConf::OnDdxUiChange( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    if ( suppressDdxFromUi_ )
    {
        return;
    }

    if ( nID == IDC_EDIT_PANEL_NAME )
    {
        panelNameDdx_->ReadFromUi();
    }
    OnDataChanged();
}

LRESULT CDialogConf::OnCloseCmd( WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/ )
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
        OnDataChangedImpl(); ///< mark as changed to reload the panel
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
            const int ret = qwr::ui::MessageBoxCentered( m_hWnd, L"Do you want to apply your changes?", L"Panel configuration", MB_ICONWARNING | MB_SETFOREGROUND | MB_YESNOCANCEL );
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

void CDialogConf::OnParentNotify( UINT message, UINT /*nChildID*/, LPARAM lParam )
{
    if ( WM_DESTROY == message && pcCurTab_ && reinterpret_cast<HWND>( lParam ) == static_cast<HWND>( *pcCurTab_ ) )
    {
        pcCurTab_ = nullptr;
    }
}

LRESULT CDialogConf::OnSelectionChanged( LPNMHDR /*pNmhdr*/ )
{
    activeTabIdx_ = cTabs_.GetCurSel();
    if ( !MaybeInitializeSmpPackageTab() )
    {
        activeTabIdx_ = GetTabIdx( Tab::script );
        cTabs_.SetCurSel( activeTabIdx_ );
    }
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

void CDialogConf::OnStartEditPanelName( UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/ )
{
    CButton edit{ GetDlgItem( IDC_BUTTON_EDIT_PANEL_NAME ) };
    edit.EnableWindow( false );
    edit.ShowWindow( SW_HIDE );

    CButton commit{ GetDlgItem( IDC_BUTTON_COMMIT_PANEL_NAME ) };
    commit.EnableWindow( true );
    commit.ShowWindow( SW_SHOWNOACTIVATE );

    CEdit panelName{ GetDlgItem( IDC_EDIT_PANEL_NAME ) };
    panelName.ModifyStyle( 0, WS_TABSTOP, 0 );
    panelName.SetReadOnly( FALSE );
    panelName.SetFocus();
}

void CDialogConf::OnCommitPanelName( UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/ )
{
    DisablePanelNameControls();
    if ( localConfig_.panelSettings.id.empty() )
    {
        localConfig_.panelSettings.id = qwr::unicode::ToU8( utils::GuidToStr( utils::GenerateGuid() ) );

        suppressDdxFromUi_ = true;
        const auto ddxSuppress = qwr::final_action( [&] { suppressDdxFromUi_ = false; } );
        panelNameDdx_->WriteToUi();
    }
}

LRESULT CDialogConf::OnHelp( WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/ )
{
    ShellExecute( nullptr, L"open", path::JsDocsIndex().c_str(), nullptr, nullptr, SW_SHOW );
    return 0;
}

void CDialogConf::DisablePanelNameControls()
{
    CButton edit{ GetDlgItem( IDC_BUTTON_EDIT_PANEL_NAME ) };
    edit.EnableWindow( true );
    edit.ShowWindow( SW_SHOWNOACTIVATE );

    CButton commit{ GetDlgItem( IDC_BUTTON_COMMIT_PANEL_NAME ) };
    commit.EnableWindow( false );
    commit.ShowWindow( SW_HIDE );

    CEdit panelName{ GetDlgItem( IDC_EDIT_PANEL_NAME ) };
    panelName.ModifyStyle( WS_TABSTOP, 0, 0 );
    panelName.SetReadOnly( TRUE );
}

void CDialogConf::DoFullDdxToUi()
{
    if ( !this->m_hWnd )
    {
        return;
    }

    panelNameDdx_->WriteToUi();
}

void CDialogConf::OnDataChangedImpl( bool hasChanged )
{
    hasChanged_ = hasChanged;
    CButton{ GetDlgItem( IDAPPLY ) }.EnableWindow( hasChanged );
}

void CDialogConf::InitializeLocalData()
{
    oldConfig_ = pParent_->GetPanelConfig();
    localConfig_ = oldConfig_;

    constexpr std::string_view kOverridenSuffix = " (overriden by script)";
    if ( pParent_->IsPanelIdOverridenByScript() )
    {
        localConfig_.panelSettings.id += kOverridenSuffix;

        if ( m_hWnd )
        {
            DisablePanelNameControls();
            CButton{ GetDlgItem( IDC_BUTTON_EDIT_PANEL_NAME ) }.EnableWindow( false );
        }
    }
    else
    {
        if ( static_cast<qwr::u8string_view>( localConfig_.panelSettings.id ).ends_with( kOverridenSuffix ) )
        {
            localConfig_.panelSettings.id.resize( localConfig_.panelSettings.id.size() - kOverridenSuffix.size() );
        }
    }
}

void CDialogConf::InitializeTabData( smp::ui::CDialogConf::Tab tabId )
{
    tabs_.clear();

    tabs_.emplace_back( std::make_unique<CConfigTabScriptSource>( *this, localConfig_ ) );
    if ( config::GetSourceType( localConfig_.scriptSource ) == config::ScriptSourceType::SmpPackage )
    {
        tabs_.emplace_back( std::make_unique<CConfigTabPackage>( *this, localConfig_ ) );
    }
    tabs_.emplace_back( std::make_unique<CConfigTabAppearance>( *this, localConfig_ ) );
    tabs_.emplace_back( std::make_unique<CConfigTabProperties>( *this, localConfig_ ) );

    SetActiveTabIdx( tabId );
}

void CDialogConf::ReinitializeTabData()
{
    if ( config::GetSourceType( localConfig_.scriptSource ) == config::ScriptSourceType::SmpPackage )
    {
        tabs_.insert( tabs_.cbegin() + GetTabIdx( Tab::package ),
                      std::make_unique<CConfigTabPackage>( *this, localConfig_ ) );
    }
    else
    {
        tabs_.erase( tabs_.cbegin() + GetTabIdx( Tab::package ) );
    }
}

void CDialogConf::RefreshTabData()
{
    for ( auto& pTab: tabs_ )
    {
        pTab->Refresh();
    }
}

void CDialogConf::InitializeTabControls()
{
    cTabs_.DeleteAllItems();

    for ( const auto& pTab: tabs_ )
    {
        cTabs_.AddItem( pTab->Name() );
    }

    if ( !MaybeInitializeSmpPackageTab() )
    {
        activeTabIdx_ = GetTabIdx( Tab::script );
    }
    cTabs_.SetCurSel( activeTabIdx_ );

    CreateChildTab();
}

void CDialogConf::ReinitializeTabControls()
{
    // do not recreate the first tab

    for ( const auto i: ranges::views::ints( 1, cTabs_.GetItemCount() ) )
    {
        (void)i;
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

size_t CDialogConf::GetTabIdx( CDialogConf::Tab tabId ) const
{
    switch ( tabId )
    {
    case Tab::script:
        return 0;
    case Tab::package:
        return 1;
    case Tab::properties:
        return tabs_.size() - 1;
    default:
        assert( false );
        return 0;
    }
}

void CDialogConf::SetActiveTabIdx( CDialogConf::Tab tabId )
{
    activeTabIdx_ = GetTabIdx( tabId );
}

bool CDialogConf::MaybeInitializeSmpPackageTab()
{
    const auto packageTabIdx = GetTabIdx( Tab::package );
    if ( config::GetSourceType( localConfig_.scriptSource ) != config::ScriptSourceType::SmpPackage || activeTabIdx_ != packageTabIdx )
    {
        return true;
    }

    return static_cast<CConfigTabPackage*>( tabs_[packageTabIdx].get() )->TryResolvePackage();
}

} // namespace smp::ui
