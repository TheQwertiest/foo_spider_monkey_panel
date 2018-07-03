#pragma once

#include "host.h"

#include <js_engine/js_container.h>
#include <panel_tooltip_param.h>

// TODO: split this class somehow
class js_panel_window 
    : public ui_helpers::container_window 
    , public js_panel_vars
{
public:
    enum class PanelType 
        : unsigned int
    {
        CUI = 0,
        DUI = 1
    };

public:
    js_panel_window( PanelType instanceType );
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

public:
    GUID GetGUID();
    HDC GetHDC() const;
    HWND GetHWND() const;
    POINT& MaxSize();
    POINT& MinSize();
    int GetHeight() const;
    int GetWidth() const;
    smp::PanelTooltipParam& GetPanelTooltipParam();
    t_script_info& ScriptInfo();
    
    t_size& DlgCode();
    PanelType GetPanelType() const;
    virtual DWORD GetColourCUI( unsigned type, const GUID& guid ) = 0;
    virtual DWORD GetColourDUI( unsigned type ) = 0;
    virtual HFONT GetFontCUI( unsigned type, const GUID& guid ) = 0;
    virtual HFONT GetFontDUI( unsigned type ) = 0;

    void /*Request*/ Repaint( bool force = false );
    void /*Request*/ RepaintRect( LONG x, LONG y, LONG w, LONG h, bool force = false );
    void RefreshBackground( LPRECT lprcUpdate = nullptr );
    unsigned SetInterval( IDispatch* func, int delay );
    unsigned SetTimeout( IDispatch* func, int delay );
    void ClearIntervalOrTimeout( UINT timerId );

private:
    
    const PanelType panelType_;

    HWND hWnd_ = nullptr;
    HDC hDc_ = nullptr;

    t_script_info m_script_info; // move to JsContainer
    bool isDropTargetRegistered_ = false; // should be with gr from JsContainer

    uint32_t height_ = 0; // Used externally as well
    uint32_t width_ = 0; // Used externally as well
    HBITMAP hBitmap_ = nullptr; // used only internally
    HBITMAP hBitmapBg_ = nullptr; // used only internally

    bool isPaintPending_ = false;                // used only internally
    bool isPaintSuppressed_ = false;             // used only internally
    bool isMouseTracked_ = false;             // used only internally
    ui_selection_holder::ptr selectionHolder_;  // used only internally

    t_size dlgCode_ = 0;                            // modified only form external
    POINT maxSize_ = { INT_MAX , INT_MAX };     // modified only form external
    POINT minSize_ = { 0 , 0 };                 // modified only form external
    smp::PanelTooltipParam panelTooltipParam_ ; // modified only form external

private:
    bool script_load();
    void script_unload();
    virtual class_data& get_class_data() const override;
    void create_context();
    void delete_context();

    // Internal callbacks
    void on_context_menu( int x, int y );
    void on_erase_background();
    void on_panel_create(HWND hWnd);
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
    bool on_mouse_button_up( UINT msg, WPARAM wp, LPARAM lp );
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
