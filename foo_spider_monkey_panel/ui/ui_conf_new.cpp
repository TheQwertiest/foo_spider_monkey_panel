#include <stdafx.h>

#include "ui_conf_new.h"

#include <panel/js_panel_window.h>
#include <ui/ui_conf_tab_appearance.h>
#include <ui/ui_conf_tab_properties.h>
#include <ui/ui_conf_tab_script_source.h>

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

CDialogConfNew::CDialogConfNew( smp::panel::js_panel_window* pParent, Tab tabId )
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

bool CDialogConfNew::IsCleanSlate() const
{
    return isCleanSlate_;
}

void CDialogConfNew::OnDataChanged()
{
    OnDataChangedImpl( true );
}

void CDialogConfNew::OnScriptTypeChange()
{
    assert( !HasChanged() ); ///< all settings should've been saved or abandoned

    Apply();

    isCleanSlate_ = true;
}

bool CDialogConfNew::HasChanged()
{
    return ( hasChanged_
             || ranges::any_of( tabs_, []( const auto& pTab ) { return pTab->HasChanged(); } ) );
}

void CDialogConfNew::Apply()
{
    oldSettings_ = localSettings_;
    oldProperties_ = localProperties_;

    for ( auto& tab: tabs_ )
    {
        tab->Apply();
    }

    OnDataChangedImpl( false );

    auto updatedSettings = oldSettings_.GeneratePanelSettings();
    updatedSettings.properties = oldProperties_;
    pParent_->UpdateSettings( updatedSettings );
}

void CDialogConfNew::Revert()
{
    localSettings_ = oldSettings_;
    localProperties_ = oldProperties_;

    for ( auto& tab: tabs_ )
    {
        tab->Revert();
    }

    OnDataChangedImpl( false );
}

BOOL CDialogConfNew::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    cTabs_ = GetDlgItem( IDC_TAB_CONF );

    InitializeTabData( startingTabId_ );
    InitializeTabControls();

    CButton{ GetDlgItem( IDAPPLY ) }.EnableWindow( false );

    return TRUE; // set focus to default control
}

LRESULT CDialogConfNew::OnCloseCmd( WORD wNotifyCode, WORD wID, HWND hWndCtl )
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

void CDialogConfNew::OnParentNotify( UINT message, UINT nChildID, LPARAM lParam )
{
    if ( WM_DESTROY == message && pcCurTab_ && reinterpret_cast<HWND>( lParam ) == static_cast<HWND>( *pcCurTab_ ) )
    {
        pcCurTab_ = nullptr;
    }
}

LRESULT CDialogConfNew::OnSelectionChanged( LPNMHDR pNmhdr )
{
    activeTabIdx_ = TabCtrl_GetCurSel( GetDlgItem( IDC_TAB_CONF ) );
    CreateChildTab();

    return 0;
}

LRESULT CDialogConfNew::OnWindowPosChanged( UINT, WPARAM, LPARAM lp, BOOL& bHandled )
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

void CDialogConfNew::OnDataChangedImpl( bool hasChanged )
{
    hasChanged_ = hasChanged;
    CButton{ GetDlgItem( IDAPPLY ) }.EnableWindow( hasChanged );
}

void CDialogConfNew::InitializeTabData( smp::ui::CDialogConfNew::Tab tabId )
{
    tabs_.clear();

    tabs_.emplace_back( std::make_unique<CConfigTabScriptSource>( *this, localSettings_ ) );
    tabs_.emplace_back( std::make_unique<CConfigTabAppearance>( *this, localSettings_ ) );
    tabs_.emplace_back( std::make_unique<CConfigTabProperties>( *this, localProperties_ ) );

    switch ( tabId )
    {
    case Tab::script:
        activeTabIdx_ = 0;
        break;
    case Tab::properties:
        activeTabIdx_ = 2;
        break;
    default:
        assert( false );
        activeTabIdx_ = 0;
        break;
    }
}

void CDialogConfNew::InitializeTabControls()
{
    cTabs_.DeleteAllItems();
    for ( auto&& [i, pTab]: ranges::views::enumerate( tabs_ ) )
    {
        cTabs_.InsertItem( i, pTab->Name() );
    }

    cTabs_.SetCurSel( activeTabIdx_ );
    CreateChildTab();
}

void CDialogConfNew::CreateChildTab()
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
    pcCurTab_ = &pCurTab->Dialog();
    pCurTab->CreateTab( m_hWnd );

    EnableThemeDialogTexture( static_cast<HWND>( *pcCurTab_ ), ETDT_ENABLETAB );

    pcCurTab_->SetWindowPos( nullptr, tabRc.left, tabRc.top, tabRc.right - tabRc.left, tabRc.bottom - tabRc.top, SWP_NOZORDER );
    cTabs_.SetWindowPos( HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );

    pcCurTab_->ShowWindow( SW_SHOWNORMAL );
}

void CDialogConfNew::DestroyChildTab()
{
    if ( pcCurTab_ && static_cast<HWND>( *pcCurTab_ ) )
    {
        pcCurTab_->ShowWindow( SW_HIDE );
        pcCurTab_->DestroyWindow();
        pcCurTab_ = nullptr;
    }
}

} // namespace smp::ui
