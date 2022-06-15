#include <stdafx.h>

#include "panel_adaptor_dui.h"

#include <com_objects/drop_target_impl.h>
#include <events/event_dispatcher.h>
#include <events/event_js_callback.h>
#include <utils/colour_helpers.h>

namespace
{

// Just because I don't want to include the helpers
template <typename T>
class UiElementImpl : public ui_element
{
public:
    GUID get_guid() override
    {
        return T::g_get_guid();
    }
    GUID get_subclass() override
    {
        return T::g_get_subclass();
    }
    void get_name( pfc::string_base& out ) override
    {
        T::g_get_name( out );
    }
    ui_element_instance::ptr instantiate( HWND parent, ui_element_config::ptr cfg, ui_element_instance_callback::ptr callback ) override
    {
        PFC_ASSERT( cfg->get_guid() == get_guid() );
        service_nnptr_t<UiElementInstanceImplHelper> item = fb2k::service_new<UiElementInstanceImplHelper>( cfg, callback );
        item->initialize_window( parent );
        return item;
    }
    ui_element_config::ptr get_default_configuration() override
    {
        return T::g_get_default_configuration();
    }
    ui_element_children_enumerator_ptr enumerate_children( ui_element_config::ptr ) override
    {
        return nullptr;
    }
    bool get_description( pfc::string_base& out ) override
    {
        out = T::g_get_description();
        return true;
    }

private:
    class UiElementInstanceImplHelper : public T
    {
    public:
        UiElementInstanceImplHelper( ui_element_config::ptr cfg, ui_element_instance_callback::ptr callback )
            : T( cfg, callback )
        {
        }
    };
};

service_factory_t<UiElementImpl<smp::panel::PanelAdaptorDui>> g_panelAdaptor;

} // namespace

namespace smp::panel
{

PanelAdaptorDui::PanelAdaptorDui( ui_element_config::ptr cfg, ui_element_instance_callback::ptr callback )
    : uiCallback_( callback )
    , isEditMode_( callback->is_edit_mode_enabled() )
{
    set_configuration( cfg );
}

PanelAdaptorDui::~PanelAdaptorDui()
{
    if ( wndContainer_ )
    {
        wndContainer_->destroy();
        wndContainer_.reset();
    }
}

GUID PanelAdaptorDui::g_get_guid()
{
    return smp::guid::window_dui;
}

GUID PanelAdaptorDui::g_get_subclass()
{
    return ui_element_subclass_utility;
}

pfc::string8 PanelAdaptorDui::g_get_description()
{
    return "Customizable panel with JavaScript support.";
}

ui_element_config::ptr PanelAdaptorDui::g_get_default_configuration()
{
    ui_element_config_builder builder;
    config::PanelSettings::SaveDefault( builder.m_stream, fb2k::noAbort );
    return builder.finish( g_get_guid() );
}

void PanelAdaptorDui::g_get_name( pfc::string_base& out )
{
    out = SMP_NAME;
}

GUID PanelAdaptorDui::get_guid()
{
    return g_get_guid();
}

GUID PanelAdaptorDui::get_subclass()
{
    return g_get_subclass();
}

PanelType PanelAdaptorDui::GetPanelType() const
{
    return PanelType::DUI;
}

DWORD PanelAdaptorDui::GetColour( unsigned type, const GUID& guid )
{
    const auto& guidToQuery = [type, &guid] {
        // Take care when changing this array:
        // guid indexes are part of SMP API
        const std::array<const GUID*, 4> guids = {
            &ui_color_text,
            &ui_color_background,
            &ui_color_highlight,
            &ui_color_selection,
        };

        if ( guid != pfc::guid_null )
        {
            return guid;
        }
        else if ( type < guids.size() )
        {
            return *guids[type];
        }
        else
        {
            return pfc::guid_null;
        }
    }();

    t_ui_color colour = 0; ///< black
    if ( guidToQuery != pfc::guid_null )
    {
        colour = uiCallback_->query_std_color( guidToQuery );
    }

    return smp::colour::ColorrefToArgb( colour );
}

HFONT PanelAdaptorDui::GetFont( unsigned type, const GUID& guid )
{
    const auto& guidToQuery = [type, &guid] {
        // Take care when changing this array:
        // guid indexes are part of SMP API
        const std::array<const GUID*, 6> guids = {
            &ui_font_default,
            &ui_font_tabs,
            &ui_font_lists,
            &ui_font_playlists,
            &ui_font_statusbar,
            &ui_font_console,
        };

        if ( guid != pfc::guid_null )
        {
            return guid;
        }
        else if ( type < guids.size() )
        {
            return *guids[type];
        }
        else
        {
            return pfc::guid_null;
        }
    }();

    return ( guidToQuery != pfc::guid_null ? uiCallback_->query_font_ex( guidToQuery ) : nullptr );
}

LRESULT PanelAdaptorDui::OnMessage( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    switch ( msg )
    {
    case WM_RBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
    case WM_CONTEXTMENU:
    {
        if ( isEditMode_ )
        {
            return DefWindowProc( hwnd, msg, wp, lp );
        }
        break;
    }
    case static_cast<UINT>( smp::MiscMessage::size_limit_changed ):
    {
        OnSizeLimitChanged( wp );
        return 0;
    }
    default:
        break;
    }

    assert( wndContainer_ );
    return wndContainer_->OnMessage( hwnd, msg, wp, lp );
}

void PanelAdaptorDui::OnSizeLimitChanged( LPARAM )
{
    uiCallback_->on_min_max_info_change();
}

HWND PanelAdaptorDui::get_wnd()
{
    assert( wndContainer_ );
    return wndContainer_->get_wnd();
}

bool PanelAdaptorDui::edit_mode_context_menu_get_description( unsigned, unsigned, pfc::string_base& )
{
    return false;
}

bool PanelAdaptorDui::edit_mode_context_menu_test( const POINT&, bool )
{
    return true;
}

ui_element_config::ptr PanelAdaptorDui::get_configuration()
{
    ui_element_config_builder builder;
    if ( wndContainer_ )
    {
        wndContainer_->SaveSettings( builder.m_stream, fb2k::noAbort );
    }
    return builder.finish( g_get_guid() );
}

void PanelAdaptorDui::edit_mode_context_menu_build( const POINT& p_point, bool, HMENU p_menu, unsigned p_id_base )
{
    assert( wndContainer_ );
    wndContainer_->GenerateContextMenu( p_menu, p_point.x, p_point.y, p_id_base );
}

void PanelAdaptorDui::edit_mode_context_menu_command( const POINT&, bool, unsigned p_id, unsigned p_id_base )
{
    assert( wndContainer_ );
    wndContainer_->ExecuteContextMenu( p_id, p_id_base );
}

void PanelAdaptorDui::notify( const GUID& p_what, t_size, const void*, t_size )
{
    if ( p_what == ui_element_notify_edit_mode_changed )
    {
        notify_is_edit_mode_changed( uiCallback_->is_edit_mode_enabled() );
    }
    else if ( p_what == ui_element_notify_font_changed )
    {
        assert( wndContainer_ );
        EventDispatcher::Get().PutEvent( wndContainer_->GetHWND(), GenerateEvent_JsCallback( EventId::kUiFontChanged ) );
    }
    else if ( p_what == ui_element_notify_colors_changed )
    {
        assert( wndContainer_ );
        EventDispatcher::Get().PutEvent( wndContainer_->GetHWND(), GenerateEvent_JsCallback( EventId::kUiColoursChanged ) );
    }
}

void PanelAdaptorDui::set_configuration( ui_element_config::ptr data )
{
    ui_element_config_parser parser( data );

    cachedPanelSettings_ = PanelWindow::LoadSettings( parser.m_stream, parser.get_remaining(), fb2k::noAbort );
    if ( wndContainer_ )
    {
        // FIX: If window already created, DUI won't destroy it and create it again.
        wndContainer_->InitSettings( cachedPanelSettings_, !!wndContainer_->GetHWND() );
    }
}

void PanelAdaptorDui::initialize_window( HWND parent )
{
    wndContainer_ = std::make_unique<PanelWindow>( *this );
    wndContainer_->InitSettings( cachedPanelSettings_, false );
    wndContainer_->create( parent );
}

void PanelAdaptorDui::notify_is_edit_mode_changed( bool enabled )
{
    isEditMode_ = enabled;
}

} // namespace smp::panel
