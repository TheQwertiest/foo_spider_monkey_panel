#include <stdafx.h>

#include "panel_window_cui.h"

#include <com_objects/drop_target_impl.h>
#include <events/event_dispatcher.h>
#include <events/event_js_callback.h>
#include <panel/panel_window.h>
#include <utils/colour_helpers.h>

namespace
{
uie::window_factory<smp::panel::js_panel_window_cui> g_js_panel_window_cui;
} // namespace

namespace smp::panel
{

js_panel_window_cui::js_panel_window_cui()
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
        ShowWindow( wndContainer_->GetHWND(), SW_HIDE );
        SetParent( wndContainer_->GetHWND(), parent );
        m_host->relinquish_ownership( wndContainer_->GetHWND() );
        m_host = host;

        SetWindowPos( wndContainer_->GetHWND(), nullptr, p_position.x, p_position.y, p_position.cx, p_position.cy, SWP_NOZORDER );
    }
    else
    {
        m_host = host; //store interface to host
        wndContainer_ = std::make_unique<js_panel_window>(
            PanelType::CUI,
            *this );
        wndContainer_->InitSettings( panel_settings_, false );
        wndContainer_->create( parent, p_position.x, p_position.y, p_position.cx, p_position.cy );
    }

    return get_wnd();
}

HWND js_panel_window_cui::get_wnd() const
{
    assert( wndContainer_ );
    return wndContainer_->get_wnd();
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

    assert( wndContainer_ );
    return wndContainer_->on_message( hwnd, msg, wp, lp );
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
    wndContainer_->ShowConfigure( parent );
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
    assert( wndContainer_ );
    wndContainer_->destroy();
    wndContainer_.reset();
    m_host.release();
}

void js_panel_window_cui::get_category( pfc::string_base& out ) const
{
    out = "Panels";
}

void js_panel_window_cui::get_config( stream_writer* writer, abort_callback& abort ) const
{
    if ( wndContainer_ )
    {
        wndContainer_->SaveSettings( *writer, abort );
    }
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
    assert( wndContainer_ );
    EventDispatcher::Get().PutEvent( wndContainer_->GetHWND(), GenerateEvent_JsCallback( EventId::kUiColoursChanged ) );
}

void js_panel_window_cui::on_font_changed( t_size ) const
{
    assert( wndContainer_ );
    EventDispatcher::Get().PutEvent( wndContainer_->GetHWND(), GenerateEvent_JsCallback( EventId::kUiFontChanged ) );
}

void js_panel_window_cui::set_config( stream_reader* reader, t_size size, abort_callback& abort )
{
    panel_settings_ = js_panel_window::LoadSettings( *reader, size, abort );
    if ( wndContainer_ )
    {
        wndContainer_->InitSettings( panel_settings_, false );
    }
}

void js_panel_window_cui::notify_size_limit_changed( LPARAM lp )
{
    assert( wndContainer_ );
    m_host->on_size_limit_change( wndContainer_->GetHWND(), lp );
}

} // namespace smp::panel
