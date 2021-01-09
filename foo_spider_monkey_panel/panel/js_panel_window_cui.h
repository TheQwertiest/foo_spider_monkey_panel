#pragma once

#include <panel/js_panel_window.h>

namespace smp::panel
{

class js_panel_window_cui
    : public js_panel_window
    , public uie::window
    , public cui::fonts::common_callback
    , public cui::colours::common_callback
{
public:
    js_panel_window_cui();

protected:
    // js_panel_window
    DWORD GetColour( unsigned type, const GUID& guid ) override;
    HFONT GetFont( unsigned type, const GUID& guid ) override;
    LRESULT on_message( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp ) override;

    // uie::window
    HWND create_or_transfer_window( HWND parent, const uie::window_host_ptr& host, const ui_helpers::window_position_t& p_position ) override;
    [[nodiscard]] HWND get_wnd() const override;
    [[nodiscard]] bool have_config_popup() const override;
    [[nodiscard]] bool is_available( const uie::window_host_ptr& p ) const override;
    bool show_config_popup( HWND parent ) override;
    [[nodiscard]] const GUID& get_extension_guid() const override;
    [[nodiscard]] unsigned get_type() const override;
    void destroy_window() override;
    void get_category( pfc::string_base& out ) const override;
    void get_config( stream_writer* writer, abort_callback& abort ) const override;
    void get_name( pfc::string_base& out ) const override;
    void set_config( stream_reader* reader, t_size size, abort_callback& abort ) override;

    // cui::colours::common_callback
    void on_colour_changed( t_size mask ) const override;
    void on_bool_changed( t_size mask ) const override;

    // cui::fonts::common_callback
    void on_font_changed( t_size mask ) const override;

private:
    // js_panel_window
    void notify_size_limit_changed( LPARAM lp ) override;

    using t_parent = js_panel_window;
    uie::window_host_ptr m_host;
};

} // namespace smp::panel
