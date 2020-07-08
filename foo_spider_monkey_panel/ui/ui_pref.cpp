#include <stdafx.h>

#include "ui_pref.h"

#include <ui/scintilla/sci_prop_sets.h>
#include <ui/ui_name_value_edit.h>
#include <utils/array_x.h>
#include <utils/file_helpers.h>

#include <filesystem>

namespace fs = std::filesystem;

using namespace smp::ui::sci;

namespace
{

class js_preferences_page_impl
    : public preferences_page_v3
{
public:
    const char* get_name() override
    {
        return SMP_NAME;
    }

    GUID get_guid() override
    {
        return smp::guid::ui_pref;
    }

    GUID get_parent_guid() override
    {
        return preferences_page::guid_tools;
    }

    bool get_help_url( pfc::string_base& p_out ) override
    {
        p_out = "https://github.com/TheQwertiest/foo_spider_monkey_panel/wiki";
        return true;
    }

    preferences_page_instance::ptr instantiate( HWND parent, preferences_page_callback::ptr callback ) override
    {
        auto p = fb2k::service_new<smp::ui::CDialogPref>( callback );
        p->Create( parent );
        return p;
    }
};

preferences_page_factory_t<js_preferences_page_impl> g_pref;

} // namespace

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

CDialogPref::CDialogPref( preferences_page_callback::ptr callback )
    : m_callback( callback )
{
}

BOOL CDialogPref::OnInitDialog( HWND, LPARAM )
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

void CDialogPref::LoadProps( bool reset )
{
    if ( reset )
    {
        g_scintillaCfg.reset();
    }

    const auto& prop_sets = g_scintillaCfg.val();

    m_props.DeleteAllItems();

    for ( auto&& [i, prop]: ranges::views::enumerate( prop_sets ) )
    {
        m_props.AddItem( i, 0, smp::unicode::ToWide( prop.key ).c_str() );
        m_props.AddItem( i, 1, smp::unicode::ToWide( prop.val ).c_str() );
    }

    OnChanged();
}

LRESULT CDialogPref::OnPropNMDblClk( LPNMHDR pnmh )
{
    //for ListView - (LPNMITEMACTIVATE)pnmh
    //for StatusBar	- (LPNMMOUSE)pnmh
    auto pniv = reinterpret_cast<LPNMITEMACTIVATE>( pnmh );

    if ( pniv->iItem >= 0 )
    {
        auto& prop_sets = g_scintillaCfg.val();

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
            m_props.SetItemText( pniv->iItem, 1, smp::unicode::ToWide( newVal ).c_str() );
            DoDataExchange();
        }
    }

    return 0;
}

std::u8string CDialogPref::uGetItemText( int nItem, int nSubItem )
{
    std::wstring buffer;

    auto size = m_props.GetItemText( nItem, nSubItem, buffer.data(), 0 );
    buffer.resize( size + 1 );
    (void)m_props.GetItemText( nItem, nSubItem, buffer.data(), size );
    buffer.resize( wcslen( buffer.c_str() ) );

    return smp::unicode::ToU8( buffer );
}

void CDialogPref::OnButtonExportBnClicked( WORD, WORD, HWND )
{
    fs::path path( smp::file::FileDialog( L"Save as", true, k_DialogExtFilter, L"cfg" ) );
    if ( !path.empty() )
    {
        path = path.lexically_normal();
        g_scintillaCfg.export_to_file( path.wstring().c_str() );
    }
}

void CDialogPref::OnButtonImportBnClicked( WORD, WORD, HWND )
{
    fs::path path( smp::file::FileDialog( L"Import from", false, k_DialogExtFilter, L"cfg" ) );
    if ( !path.empty() )
    {
        path = path.lexically_normal();
        g_scintillaCfg.import_from_file( path.u8string().c_str() );
    }

    LoadProps();
}

void CDialogPref::OnEditChange( WORD, WORD, HWND )
{
    OnChanged();
}

void CDialogPref::OnChanged()
{
    m_callback->on_state_changed();
}

HWND CDialogPref::get_wnd()
{
    return m_hWnd;
}

t_uint32 CDialogPref::get_state()
{
    return preferences_state::resettable;
}

void CDialogPref::apply()
{
    OnChanged();
}

void CDialogPref::reset()
{
    LoadProps( true );
}

} // namespace smp::ui
