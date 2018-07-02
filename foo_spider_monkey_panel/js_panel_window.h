#pragma once

#include "host.h"

#include <js_engine/js_container.h>

class js_panel_window 
    : public ui_helpers::container_window 
    //,public HostComm
    , public js_panel_vars
{
public:
    js_panel_window();
    virtual ~js_panel_window();
    void update_script(const char* code = nullptr);

    void JsEngineFail( std::string_view errorText );
protected:
    LRESULT on_message(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    bool show_configure_popup(HWND parent);
    bool show_property_popup(HWND parent);
    static void build_context_menu(HMENU menu, int x, int y, int id_base);
    virtual void notify_size_limit_changed(LPARAM lp) = 0;
    void execute_context_menu_command(int id, int id_base);

private:    
    mozjs::JsContainer jsContainer_;    

    CComPtr<IDropTargetImpl> m_drop_target; // keep
    IGdiGraphicsPtr m_gr_wrap; // currently in JsContainer: extract from there to a proper place (ConstAfterScriptLoad)
    ScriptHost* m_script_host; // remove

private:
    enum class PanelType
    {
        CUI = 0,
        DUI = 1
    };

    struct AlwaysConst
    {// Used externally as well
        PanelType m_instance_type;
    };

    struct ConstAfterPanelLoad
    {// Used externally as well
        HWND m_hwnd;
        HDC m_hdc;
    };

    struct ConstAfterScriptLoad
    {
        t_script_info m_script_info; // move to JsContainer
        bool m_is_droptarget_registered; // should be with gr from JsContainer
    };

    struct GraphicsOnSizeAndCreation
    {
        uint32_t m_height; // Used externally as well
        uint32_t m_width; // Used externally as well
        HBITMAP m_gr_bmp; // used only internally
        HBITMAP m_gr_bmp_bk; // used only internally
    };

    struct DynamicDataModifiedOnlyFromInternal
    {// used only internally
        bool m_paint_pending;
        bool m_suppress_drawing;
        bool m_is_mouse_tracked;
        ui_selection_holder::ptr m_selection_holder;
    };

    struct DynamicDataModifiedOnlyFromExternal
    {// used internally and externally
        t_size m_dlg_code;
        POINT m_max_size;
        POINT m_min_size;
        panel_tooltip_param_ptr m_panel_tooltip_param_ptr;
    };  

private:
    bool on_mouse_button_up(UINT msg, WPARAM wp, LPARAM lp);
    bool script_load();
    void script_unload();
    virtual class_data& get_class_data() const;
    void create_context();
    void delete_context();

    // Internal callbacks
    void on_context_menu( int x, int y );
    void on_erase_background();
    void on_panel_create();
    void on_panel_destroy();
    void on_script_error();

    // JS callbacks
    void on_always_on_top_changed(WPARAM wp);
    void on_char( WPARAM wp );
    void on_colours_changed();
    void on_cursor_follow_playback_changed(WPARAM wp);
    void on_dsp_preset_changed();
    void on_focus_changed(bool isFocused);
    void on_font_changed();
    void on_get_album_art_done(LPARAM lp);
    void on_item_focus_change(WPARAM wp);
    void on_item_played(WPARAM wp);
    void on_key_down( WPARAM wp );
    void on_key_up( WPARAM wp );
    void on_library_items_added(WPARAM wp);
    void on_library_items_changed(WPARAM wp);
    void on_library_items_removed(WPARAM wp);
    void on_load_image_done(LPARAM lp);
    void on_main_menu(WPARAM wp);
    void on_metadb_changed(WPARAM wp);
    void on_mouse_button_dblclk(UINT msg, WPARAM wp, LPARAM lp);
    void on_mouse_button_down(UINT msg, WPARAM wp, LPARAM lp);
    void on_mouse_leave();
    void on_mouse_move(WPARAM wp, LPARAM lp);
    void on_mouse_wheel(WPARAM wp);
    void on_mouse_wheel_h(WPARAM wp);
    void on_notify_data(WPARAM wp);
    void on_output_device_changed();
    void on_paint(HDC dc, LPRECT lpUpdateRect);
    void on_paint_error(HDC memdc);
    void on_paint_user(HDC memdc, LPRECT lpUpdateRect);
    void on_playback_dynamic_info();
    void on_playback_dynamic_info_track();
    void on_playback_edited(WPARAM wp);
    void on_playback_follow_cursor_changed(WPARAM wp);
    void on_playback_new_track(WPARAM wp);
    void on_playback_order_changed(WPARAM wp);
    void on_playback_pause(bool state);
    void on_playback_queue_changed(WPARAM wp);
    void on_playback_seek(WPARAM wp);
    void on_playback_starting(play_control::t_track_command cmd, bool paused);
    void on_playback_stop(play_control::t_stop_reason reason);
    void on_playback_time(WPARAM wp);
    void on_playlist_item_ensure_visible(WPARAM wp, LPARAM lp);
    void on_playlist_items_added(WPARAM wp);
    void on_playlist_items_removed(WPARAM wp, LPARAM lp);
    void on_playlist_items_reordered(WPARAM wp);
    void on_playlist_items_selection_change();
    void on_playlist_stop_after_current_changed(WPARAM wp);
    void on_playlist_switch();
    void on_playlists_changed();
    void on_replaygain_mode_changed(WPARAM wp);
    void on_selection_changed();
    void on_size(uint32_t w, uint32_t h);
    void on_volume_change(WPARAM wp);
};
