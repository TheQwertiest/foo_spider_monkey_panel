#include <stdafx.h>

#include "js_panel_window_cui.h"

#include <com_objects/drop_target_impl.h>
#include <events/event_dispatcher.h>
#include <events/event_js_callback.h>
#include <utils/colour_helpers.h>

namespace
{
uie::window_factory<smp::panel::js_panel_window_cui> g_js_panel_window_cui;
} // namespace

namespace smp::panel
{

js_panel_window_cui::js_panel_window_cui()
    : js_panel_window( PanelType::CUI )
{
}

DWORD js_panel_window_cui::GetColour( unsigned type, const GUID& guid )
{
    COLORREF colour = 0; ///< black
    if ( type <= cui::colours::colour_active_item_frame )
    {
        cui::colours::helper helper( guid );
        colour = helper.get_colour( static_cast<cui::colours::colour_identifier_t>( type ) );
    }

    return smp::colour::ColorrefToArgb( colour );
}

HFONT js_panel_window_cui::GetFont( unsigned type, const GUID& guid )
{
    try
    {
        auto api = static_api_ptr_t<cui::fonts::manager>();
        if ( guid != pfc::guid_null )
        {
            return api->get_font( guid );
        }
        else if ( type <= cui::fonts::font_type_labels )
        {
            return api->get_font( static_cast<cui::fonts::font_type_t>( type ) );
        }
    }
    catch ( const exception_service_extension_not_found& )
    {
    }

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
    {
        if ( uie::window::g_process_keydown_keyboard_shortcuts( wp ) )
            return 0;
        break;
    }
    case WM_CREATE:
    {
        try
        {
            static_api_ptr_t<cui::fonts::manager>()->register_common_callback( this );
            static_api_ptr_t<cui::colours::manager>()->register_common_callback( this );
        }
        catch ( const exception_service_extension_not_found& )
        {
        }
        break;
    }
    case WM_DESTROY:
    {
        try
        {
            static_api_ptr_t<cui::fonts::manager>()->deregister_common_callback( this );
            static_api_ptr_t<cui::colours::manager>()->deregister_common_callback( this );
        }
        catch ( const exception_service_extension_not_found& )
        {
        }
        break;
    }
    case static_cast<UINT>( smp::MiscMessage::size_limit_changed ):
    {
        notify_size_limit_changed( wp );
        return 0;
    }
    default:
        break;
    }

    return t_parent::on_message( hwnd, msg, wp, lp );
}

bool js_panel_window_cui::have_config_popup() const
{
    return true;
}

bool js_panel_window_cui::is_available( const uie::window_host_ptr& ) const
{
    return true;
}

bool js_panel_window_cui::show_config_popup( HWND parent )
{
    ShowConfigure( parent );
    return true;
}

const GUID& js_panel_window_cui::get_extension_guid() const
{
    return smp::guid::window_cui;
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
    SaveSettings( *writer, abort );
}

void js_panel_window_cui::get_name( pfc::string_base& out ) const
{
    out = SMP_NAME;
}

void js_panel_window_cui::on_bool_changed( t_size ) const
{
}

void js_panel_window_cui::on_colour_changed( t_size ) const
{
    EventDispatcher::Get().PutEvent( t_parent::GetHWND(), GenerateEvent_JsCallback( EventId::kUiColoursChanged ) );
}

void js_panel_window_cui::on_font_changed( t_size ) const
{
    EventDispatcher::Get().PutEvent( t_parent::GetHWND(), GenerateEvent_JsCallback( EventId::kUiFontChanged ) );
}

void js_panel_window_cui::set_config( stream_reader* reader, t_size size, abort_callback& abort )
{
    LoadSettings( *reader, size, abort, false );
}

void js_panel_window_cui::notify_size_limit_changed( LPARAM lp )
{
    m_host->on_size_limit_change( t_parent::GetHWND(), lp );
}

} // namespace smp::panel
