#include <stdafx.h>

#include "ui_conf_tab_appearance.h"

#include <ui/ui_conf.h>

namespace
{

int GetEdgeIdFromEnum( smp::config::EdgeStyle edgeStyle )
{
    switch ( edgeStyle )
    {
    case smp::config::EdgeStyle::NoEdge:
    {
        return IDC_RADIO_EDGE_NO;
    }
    case smp::config::EdgeStyle::SunkenEdge:
    {
        return IDC_RADIO_EDGE_SUNKEN;
    }
    case smp::config::EdgeStyle::GreyEdge:
    {
        return IDC_RADIO_EDGE_GREY;
    }
    default:
    {
        assert( false );
        return IDC_RADIO_EDGE_NO;
    }
    }
}

smp::config::EdgeStyle GetEdgeEnumFromId( int edgeStyleId )
{
    switch ( edgeStyleId )
    {
    case IDC_RADIO_EDGE_NO:
    {
        return smp::config::EdgeStyle::NoEdge;
    }
    case IDC_RADIO_EDGE_SUNKEN:
    {
        return smp::config::EdgeStyle::SunkenEdge;
    }
    case IDC_RADIO_EDGE_GREY:
    {
        return smp::config::EdgeStyle::GreyEdge;
    }
    default:
    {
        assert( false );
        return smp::config::EdgeStyle::NoEdge;
    }
    }
}

} // namespace

namespace smp::ui
{

CConfigTabAppearance::CConfigTabAppearance( CDialogConf& parent, config::ParsedPanelSettings& settings )
    : parent_( parent )
    , edgeStyle_( settings.edgeStyle )
    , isPseudoTransparent_( settings.isPseudoTransparent )
    , ddx_( {
          qwr::ui::CreateUiDdx<qwr::ui::UiDdx_CheckBox>( isPseudoTransparent_, IDC_CHECK_PSEUDOTRANSPARENT ),
          qwr::ui::CreateUiDdx<qwr::ui::UiDdx_RadioRange>( edgeStyleId_,
                                                           std::initializer_list<int>{
                                                               IDC_RADIO_EDGE_NO,
                                                               IDC_RADIO_EDGE_SUNKEN,
                                                               IDC_RADIO_EDGE_GREY,
                                                           } ),
      } )
{
    InitializeLocalOptions();
}

HWND CConfigTabAppearance::CreateTab( HWND hParent )
{
    return Create( hParent );
}

CDialogImplBase& CConfigTabAppearance::Dialog()
{
    return *this;
}

const wchar_t* CConfigTabAppearance::Name() const
{
    return L"Appearance";
}

bool CConfigTabAppearance::HasChanged()
{
    return false;
}

void CConfigTabAppearance::Apply()
{
}

void CConfigTabAppearance::Revert()
{
}

void CConfigTabAppearance::Refresh()
{
}

BOOL CConfigTabAppearance::OnInitDialog( HWND /*hwndFocus*/, LPARAM /*lParam*/ )
{
    for ( auto& ddx: ddx_ )
    {
        ddx->SetHwnd( m_hWnd );
    }

    DoFullDdxToUi();

    return TRUE; // set focus to default control
}

void CConfigTabAppearance::OnDdxUiChange( UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/ )
{
    auto it = ranges::find_if( ddx_, [nID]( auto& ddx ) {
        return ddx->IsMatchingId( nID );
    } );

    if ( ddx_.end() != it )
    {
        ( *it )->ReadFromUi();
    }

    switch ( nID )
    {
    case IDC_RADIO_EDGE_NO:
    case IDC_RADIO_EDGE_SUNKEN:
    case IDC_RADIO_EDGE_GREY:
    {
        edgeStyle_ = GetEdgeEnumFromId( edgeStyleId_ );
        break;
    }
    default:
        break;
    }

    parent_.OnDataChanged();
}

void CConfigTabAppearance::DoFullDdxToUi()
{
    if ( !this->m_hWnd )
    {
        return;
    }

    for ( auto& ddx: ddx_ )
    {
        ddx->WriteToUi();
    }
}

void CConfigTabAppearance::InitializeLocalOptions()
{
    edgeStyleId_ = GetEdgeIdFromEnum( edgeStyle_ );
}

} // namespace smp::ui
