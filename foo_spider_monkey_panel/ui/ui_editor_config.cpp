#include <stdafx.h>

#include "ui_editor_config.h"

#include <ui/scintilla/sci_config.h>
#include <ui/ui_name_value_edit.h>

#include <qwr/file_helpers.h>

#include <filesystem>

namespace fs = std::filesystem;

namespace
{

constexpr auto k_DialogExtFilter = std::to_array<COMDLG_FILTERSPEC>(
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

    SetWindowTheme( propertiesListView_.m_hWnd, L"explorer", nullptr );

    propertiesListView_.SetExtendedListViewStyle( LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER );
    propertiesListView_.AddColumn( L"Name", 0 );
    propertiesListView_.SetColumnWidth( 0, 150 );
    propertiesListView_.AddColumn( L"Value", 1 );
    propertiesListView_.SetColumnWidth( 1, 310 );
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
    auto pniv = reinterpret_cast<LPNMITEMACTIVATE>( pnmh );

    if ( pniv->iItem < 0 )
    {
        return 0;
    }

    const auto key = this->GetItemTextStr( pniv->iItem, 0 );
    const auto val = this->GetItemTextStr( pniv->iItem, 1 );

    CNameValueEdit dlg( key.c_str(), val.c_str(), "Edit property" );
    if ( IDOK == dlg.DoModal( m_hWnd ) )
    {
        const auto newVal = dlg.GetValue();

        // Save
        auto& prop_sets = config::sci::props.val();
        auto it = ranges::find_if( prop_sets, [&key]( const auto& elem ) { return ( elem.key == key ); } );
        if ( it != prop_sets.end() )
        {
            it->val = newVal;
        }

        // Update list
        propertiesListView_.SetItemText( pniv->iItem, 1, qwr::unicode::ToWide( newVal ).c_str() );
        DoDataExchange();
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

    propertiesListView_.DeleteAllItems();

    for ( auto&& [i, prop]: ranges::views::enumerate( prop_sets ) )
    {
        propertiesListView_.AddItem( i, 0, qwr::unicode::ToWide( prop.key ).c_str() );
        propertiesListView_.AddItem( i, 1, qwr::unicode::ToWide( prop.val ).c_str() );
    }
}

qwr::u8string CDialogEditorConfig::GetItemTextStr( int nItem, int nSubItem )
{
    constexpr size_t kBufferLen = 256;
    std::wstring buffer;
    buffer.resize( kBufferLen );

    auto size = propertiesListView_.GetItemText( nItem, nSubItem, buffer.data(), buffer.size() );
    // size == wcslen(buffer.c_str())
    buffer.resize( size );

    return qwr::unicode::ToU8( buffer );
}

} // namespace smp::ui
