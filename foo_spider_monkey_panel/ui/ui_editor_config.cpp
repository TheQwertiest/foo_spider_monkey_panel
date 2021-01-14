#include <stdafx.h>

#include "ui_editor_config.h"

#include <ui/scintilla/sci_config.h>
#include <ui/ui_name_value_edit.h>
#include <utils/array_x.h>

#include <qwr/file_helpers.h>

#include <filesystem>

namespace fs = std::filesystem;

namespace
{

constexpr auto k_DialogExtFilter = smp::to_array<COMDLG_FILTERSPEC>(
    {
        { L"Configuration files", L"*.cfg" },
        { L"All files", L"*.*" },
    } );

} // namespace

namespace smp::ui
{

CDialogEditorConfig::CDialogEditorConfig()
{
}

BOOL CDialogEditorConfig::OnInitDialog( HWND, LPARAM )
{
    DoDataExchange();

    SetWindowTheme( m_props.m_hWnd, L"explorer", nullptr );

    m_props.SetExtendedListViewStyle( LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER );
    m_props.AddColumn( L"Name", 0 );
    m_props.SetColumnWidth( 0, 150 );
    m_props.AddColumn( L"Value", 1 );
    m_props.SetColumnWidth( 1, 310 );
    LoadProps();

    return TRUE; // set focus to default control
}

LRESULT CDialogEditorConfig::OnCloseCmd( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{
    switch ( wID )
    {
    case IDOK:
    {
        EndDialog( IDOK );
        break;
    }
    case IDCANCEL:
    {
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

void CDialogEditorConfig::OnButtonReset( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{
    LoadProps( true );
}

void CDialogEditorConfig::OnButtonExportBnClicked( WORD, WORD, HWND )
{
    qwr::file::FileDialogOptions fdOpts{};
    fdOpts.savePathGuid = guid::dialog_path;
    fdOpts.filterSpec.assign( k_DialogExtFilter.begin(), k_DialogExtFilter.end() );
    fdOpts.defaultExtension = L"cfg";

    const auto path_opt = qwr::file::FileDialog( L"Save as", true, fdOpts );
    if ( !path_opt || path_opt->empty() )
    {
        return;
    }

    const auto path = path_opt->lexically_normal();
    config::sci::props.export_to_file( path.c_str() );
}

void CDialogEditorConfig::OnButtonImportBnClicked( WORD, WORD, HWND )
{
    qwr::file::FileDialogOptions fdOpts{};
    fdOpts.savePathGuid = guid::dialog_path;
    fdOpts.filterSpec.assign( k_DialogExtFilter.begin(), k_DialogExtFilter.end() );
    fdOpts.defaultExtension = L"cfg";

    const auto path_opt = qwr::file::FileDialog( L"Import from", false, fdOpts );
    if ( !path_opt || path_opt->empty() )
    {
        return;
    }

    const auto path = path_opt->lexically_normal();
    config::sci::props.import_from_file( path.u8string().c_str() );

    LoadProps();
}

LRESULT CDialogEditorConfig::OnPropNMDblClk( LPNMHDR pnmh )
{
    //for ListView - (LPNMITEMACTIVATE)pnmh
    //for StatusBar	- (LPNMMOUSE)pnmh
    auto pniv = reinterpret_cast<LPNMITEMACTIVATE>( pnmh );

    if ( pniv->iItem >= 0 )
    {
        auto& prop_sets = config::sci::props.val();

        const auto key = this->uGetItemText( pniv->iItem, 0 );
        const auto val = this->uGetItemText( pniv->iItem, 1 );

        if ( !modal_dialog_scope::can_create() )
        {
            return 0;
        }

        modal_dialog_scope scope( m_hWnd );

        CNameValueEdit dlg( key.c_str(), val.c_str() );

        if ( IDOK == dlg.DoModal( m_hWnd ) )
        {
            const auto newVal = dlg.GetValue();

            // Save
            auto it = ranges::find_if( prop_sets, [&key]( const auto& elem ) { return ( elem.key == key ); } );
            if ( it != prop_sets.end() )
            {
                it->val = newVal;
            }

            // Update list
            m_props.SetItemText( pniv->iItem, 1, qwr::unicode::ToWide( newVal ).c_str() );
            DoDataExchange();
        }
    }

    return 0;
}

void CDialogEditorConfig::LoadProps( bool reset )
{
    if ( reset )
    {
        config::sci::props.reset();
    }

    const auto& prop_sets = config::sci::props.val();

    m_props.DeleteAllItems();

    for ( auto&& [i, prop]: ranges::views::enumerate( prop_sets ) )
    {
        m_props.AddItem( i, 0, qwr::unicode::ToWide( prop.key ).c_str() );
        m_props.AddItem( i, 1, qwr::unicode::ToWide( prop.val ).c_str() );
    }
}

std::u8string CDialogEditorConfig::uGetItemText( int nItem, int nSubItem )
{
    std::wstring buffer;

    auto size = m_props.GetItemText( nItem, nSubItem, buffer.data(), 0 );
    buffer.resize( size + 1 );
    (void)m_props.GetItemText( nItem, nSubItem, buffer.data(), size );
    buffer.resize( wcslen( buffer.c_str() ) );

    return qwr::unicode::ToU8( buffer );
}

} // namespace smp::ui
