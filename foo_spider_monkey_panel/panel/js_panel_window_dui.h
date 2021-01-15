#pragma once

#include <panel/js_panel_window.h>

namespace smp::panel
{

class js_panel_window_dui
    : public js_panel_window
    , public ui_element_instance
{
public:
    js_panel_window_dui( ui_element_config::ptr cfg, ui_element_instance_callback::ptr callback );
    ~js_panel_window_dui() override;

    static GUID g_get_guid();
    static GUID g_get_subclass();
    static pfc::string8 g_get_description();
    static ui_element_config::ptr g_get_default_configuration();
    static void g_get_name( pfc::string_base& out );

    // js_panel_window
    DWORD GetColour( unsigned type, const GUID& guid ) override;
    HFONT GetFont( unsigned type, const GUID& guid ) override;
    LRESULT on_message( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp ) override;

    // ui_element_instance
    GUID get_guid() override;
    GUID get_subclass() override;
    HWND get_wnd() override;
    bool edit_mode_context_menu_get_description( unsigned p_id, unsigned p_id_base, pfc::string_base& p_out ) override;
    bool edit_mode_context_menu_test( const POINT& p_point, bool p_fromkeyboard ) override;
    ui_element_config::ptr get_configuration() override;
    void edit_mode_context_menu_build( const POINT& p_point, bool p_fromkeyboard, HMENU p_menu, unsigned p_id_base ) override;
    void edit_mode_context_menu_command( const POINT& p_point, bool p_fromkeyboard, unsigned p_id, unsigned p_id_base ) override;
    void notify( const GUID& p_what, t_size p_param1, const void* p_param2, t_size p_param2size ) override;
    void set_configuration( ui_element_config::ptr data ) override;

    void initialize_window( HWND parent );

private:
    // js_panel_window
    void notify_size_limit_changed( LPARAM lp ) override;

    void notify_is_edit_mode_changed( bool enabled );

private:
    using t_parent = js_panel_window;
    ui_element_instance_callback::ptr uiCallback_;
    bool isEditMode_;
};

} // namespace smp::panel
