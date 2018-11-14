#include "stdafx.h"
#include "js_panel_window.h"
#include "js_panel_window_cui.h"

#include "helpers.h"
#include "user_message.h"

namespace
{
// CUI panel instance
static uie::window_factory<smp::panel::js_panel_window_cui> g_js_panel_wndow_cui;
} // namespace

namespace smp::panel
{

DWORD js_panel_window_cui::GetColourCUI( unsigned type, const GUID& guid )
{
    if ( type <= cui::colours::colour_active_item_frame )
    {
        cui::colours::helper helper( guid );

        return helpers::convert_colorref_to_argb( helper.get_colour( (cui::colours::colour_identifier_t)type ) );
    }

    return 0;
}

DWORD js_panel_window_cui::GetColourDUI( unsigned type )
{
    return 0;
}

HFONT js_panel_window_cui::GetFontCUI( unsigned type, const GUID& guid )
{
    if ( guid == pfc::guid_null )
    {
        if ( type <= cui::fonts::font_type_labels )
        {
            try
            {
                return static_api_ptr_t<cui::fonts::manager>()->get_font( (cui::fonts::font_type_t)type );
            }
            catch ( exception_service_not_found& )
            {
                return uCreateIconFont();
            }
        }
    }
    else
    {
        cui::fonts::helper helper( guid );
        return helper.get_font();
    }

    return nullptr;
}

HFONT js_panel_window_cui::GetFontDUI( unsigned type )
{
    return nullptr;
}

HWND js_panel_window_cui::create_or_transfer_window( HWND parent, const uie::window_host_ptr& host, const ui_helpers::window_position_t& p_position )
{
    if ( m_host.is_valid() )
    {
        ShowWindow( t_parent::GetHWND(), SW_HIDE );
        SetParent( t_parent::GetHWND(), parent );
        m_host->relinquish_ownership( t_parent::GetHWND() );
        m_host = host;

        SetWindowPos( t_parent::GetHWND(), nullptr, p_position.x, p_position.y, p_position.cx, p_position.cy, SWP_NOZORDER );
    }
    else
    {
        m_host = host; //store interface to host
        create( parent, this, p_position );
    }

    return get_wnd();
}

HWND js_panel_window_cui::get_wnd() const
{
    return t_parent::get_wnd();
}

LRESULT js_panel_window_cui::on_message( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    switch ( msg )
    {
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        if ( uie::window::g_process_keydown_keyboard_shortcuts( wp ) )
            return 0;
        break;

    case WM_CREATE:
        try
        {
            static_api_ptr_t<cui::fonts::manager>()->register_common_callback( this );
            static_api_ptr_t<cui::colours::manager>()->register_common_callback( this );
        }
        catch ( ... )
        {
        }
        break;

    case WM_DESTROY:
        try
        {
            static_api_ptr_t<cui::fonts::manager>()->deregister_common_callback( this );
            static_api_ptr_t<cui::colours::manager>()->deregister_common_callback( this );
        }
        catch ( ... )
        {
        }
        break;

    case static_cast<UINT>( smp::InternalAsyncMessage::size_limit_changed ):
    {
        notify_size_limit_changed( lp );
        return 0;
    }
    }

    return t_parent::on_message( hwnd, msg, wp, lp );
}

bool js_panel_window_cui::have_config_popup() const
{
    return true;
}

bool js_panel_window_cui::is_available( const uie::window_host_ptr& p ) const
{
    return true;
}

bool js_panel_window_cui::show_config_popup( HWND parent )
{
    return show_configure_popup( parent );
}

const GUID& js_panel_window_cui::get_extension_guid() const
{
    return g_guid_smp_window_cui;
}

const uie::window_host_ptr& js_panel_window_cui::get_host() const
{
    return m_host;
}

unsigned js_panel_window_cui::get_type() const
{
    return uie::type_toolbar | uie::type_panel;
}

void js_panel_window_cui::destroy_window()
{
    destroy();
    m_host.release();
}

void js_panel_window_cui::get_category( pfc::string_base& out ) const
{
    out = "Panels";
}

void js_panel_window_cui::get_config( stream_writer* writer, abort_callback& abort ) const
{
    save_config( writer, abort );
}

void js_panel_window_cui::get_name( pfc::string_base& out ) const
{
    out = SMP_NAME;
}

void js_panel_window_cui::on_bool_changed( t_size mask ) const
{
    // TODO: may be implemented one day
}

void js_panel_window_cui::on_colour_changed( t_size mask ) const
{
    PostMessage( t_parent::GetHWND(), static_cast<UINT>( smp::PlayerMessage::ui_colours_changed ), 0, 0 );
}

void js_panel_window_cui::on_font_changed( t_size mask ) const
{
    PostMessage( t_parent::GetHWND(), static_cast<UINT>( smp::PlayerMessage::ui_font_changed ), 0, 0 );
}

void js_panel_window_cui::set_config( stream_reader* reader, t_size size, abort_callback& abort )
{
    load_config( reader, size, abort );
}

void js_panel_window_cui::notify_size_limit_changed( LPARAM lp )
{
    get_host()->on_size_limit_change( t_parent::GetHWND(), lp );
}

} // namespace smp::panel
