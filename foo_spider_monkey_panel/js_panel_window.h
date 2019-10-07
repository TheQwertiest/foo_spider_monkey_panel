#pragma once

#include <config.h>
#include <panel_info.h>
#include <panel_tooltip_param.h>
#include <user_message.h>

#include <queue>

namespace mozjs
{
class JsContainer;
}

namespace smp::panel
{

class CallbackData;

enum class PanelType : uint8_t
{
    CUI = 0,
    DUI = 1
};

// TODO: split this class somehow
class js_panel_window
    : public ui_helpers::container_window
{
public:
    js_panel_window( PanelType instanceType );
    virtual ~js_panel_window() = default;

public:
    // ui_helpers::container_window
    class_data& get_class_data() const override;

    void update_script( const char* code = nullptr );
    void JsEngineFail( const std::u8string& errorText );

protected:
    virtual void notify_size_limit_changed( LPARAM lp ) = 0;

    // ui_helpers::container_window
    LRESULT on_message( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp ) override;

    bool show_configure_popup( HWND parent );
    void show_property_popup( HWND parent );

    static void build_context_menu( HMENU menu, int x, int y, uint32_t id_base );
    void execute_context_menu_command( uint32_t id, uint32_t id_base );

private:
    std::optional<LRESULT> process_sync_messages( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp );
    std::optional<LRESULT> process_async_messages( UINT msg, WPARAM wp, LPARAM lp );
    std::optional<LRESULT> process_main_messages( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp );
    std::optional<LRESULT> process_window_messages( UINT msg, WPARAM wp, LPARAM lp );
    std::optional<LRESULT> process_callback_messages( CallbackMessage msg );
    std::optional<LRESULT> process_player_messages( PlayerMessage msg, WPARAM wp, LPARAM lp );
    std::optional<LRESULT> process_internal_sync_messages( InternalSyncMessage msg, WPARAM wp, LPARAM lp );
    std::optional<LRESULT> process_internal_async_messages( InternalAsyncMessage msg, WPARAM wp, LPARAM lp );

public:
    GUID GetGUID();
    HDC GetHDC() const;
    HWND GetHWND() const;
    POINT& MaxSize();
    POINT& MinSize();
    int GetHeight() const;
    int GetWidth() const;
    PanelTooltipParam& GetPanelTooltipParam();
    PanelInfo& ScriptInfo();

    t_size& DlgCode();
    PanelType GetPanelType() const;
    virtual DWORD GetColour( unsigned type, const GUID& guid = pfc::guid_null ) = 0;
    virtual HFONT GetFont( unsigned type, const GUID& guid = pfc::guid_null ) = 0;

    void Repaint( bool force = false );
    void RepaintRect( LONG x, LONG y, LONG w, LONG h, bool force = false );
    void RepaintRect( const RECT& rect, bool force = false );
    /// @details Calls Repaint inside
    void RepaintBackground( LPRECT lprcUpdate = nullptr );

    config::PanelSettings& GetSettings();
    const config::PanelSettings& GetSettings() const;

private:
    config::PanelSettings settings_;

    const PanelType panelType_;
    std::shared_ptr<mozjs::JsContainer> pJsContainer_;

    HWND hWnd_ = nullptr;
    HDC hDc_ = nullptr;

    PanelInfo m_script_info; // TODO: move to JsContainer

    uint32_t height_ = 0;         // Used externally as well
    uint32_t width_ = 0;          // Used externally as well
    HBITMAP hBitmap_ = nullptr;   // used only internally
    HBITMAP hBitmapBg_ = nullptr; // used only internally

    bool isBgRepaintNeeded_ = false;           // used only internally
    bool isPaintInProgress_ = false;           // used only internally
    bool isMouseTracked_ = false;              // used only internally
    ui_selection_holder::ptr selectionHolder_; // used only internally

    t_size dlgCode_ = 0;                   // modified only from external
    POINT maxSize_ = { INT_MAX, INT_MAX }; // modified only from external
    POINT minSize_ = { 0, 0 };             // modified only from external
    PanelTooltipParam panelTooltipParam_;  // modified only from external

private:
    bool script_load();
    void script_unload();
    void create_context();
    void delete_context();

    // Internal callbacks
    void on_context_menu( int x, int y );
    void on_erase_background();
    void on_panel_create( HWND hWnd );
    void on_panel_destroy();
    void on_script_error();
    void on_js_task( CallbackData& callbackData );

    // JS callbacks
    void on_always_on_top_changed( WPARAM wp );
    void on_char( WPARAM wp );
    void on_colours_changed();
    void on_cursor_follow_playback_changed( WPARAM wp );
    void on_drag_drop( LPARAM lp );
    void on_drag_enter( LPARAM lp );
    void on_drag_leave();
    void on_drag_over( LPARAM lp );
    void on_dsp_preset_changed();
    void on_focus( bool isFocused );
    void on_font_changed();
    void on_get_album_art_done( CallbackData& callbackData );
    void on_item_focus_change( CallbackData& callbackData );
    void on_item_played( CallbackData& callbackData );
    void on_key_down( WPARAM wp );
    void on_key_up( WPARAM wp );
    void on_library_items_added( CallbackData& callbackData );
    void on_library_items_changed( CallbackData& callbackData );
    void on_library_items_removed( CallbackData& callbackData );
    void on_load_image_done( CallbackData& callbackData );
    void on_main_menu( WPARAM wp );
    void on_metadb_changed( CallbackData& callbackData );
    void on_mouse_button_dblclk( UINT msg, WPARAM wp, LPARAM lp );
    void on_mouse_button_down( UINT msg, WPARAM wp, LPARAM lp );
    bool on_mouse_button_up( UINT msg, WPARAM wp, LPARAM lp );
    void on_mouse_leave();
    void on_mouse_move( WPARAM wp, LPARAM lp );
    void on_mouse_wheel( WPARAM wp );
    void on_mouse_wheel_h( WPARAM wp );
    void on_notify_data( WPARAM wp, LPARAM lp );
    void on_output_device_changed();
    void on_paint( HDC dc, LPRECT lpUpdateRect );
    void on_paint_error( HDC memdc );
    void on_paint_user( HDC memdc, LPRECT lpUpdateRect );
    void on_playback_dynamic_info();
    void on_playback_dynamic_info_track();
    void on_playback_edited( CallbackData& callbackData );
    void on_playback_follow_cursor_changed( WPARAM wp );
    void on_playback_new_track( CallbackData& callbackData );
    void on_playback_order_changed( WPARAM wp );
    void on_playback_pause( WPARAM wp );
    void on_playback_queue_changed( WPARAM wp );
    void on_playback_seek( CallbackData& callbackData );
    void on_playback_starting( WPARAM wp, LPARAM lp );
    void on_playback_stop( WPARAM wp );
    void on_playback_time( CallbackData& callbackData );
    void on_playlist_item_ensure_visible( WPARAM wp, LPARAM lp );
    void on_playlist_items_added( WPARAM wp );
    void on_playlist_items_removed( WPARAM wp, LPARAM lp );
    void on_playlist_items_reordered( WPARAM wp );
    void on_playlist_items_selection_change();
    void on_playlist_stop_after_current_changed( WPARAM wp );
    void on_playlist_switch();
    void on_playlists_changed();
    void on_replaygain_mode_changed( WPARAM wp );
    void on_selection_changed();
    void on_size( uint32_t w, uint32_t h );
    void on_volume_change( CallbackData& callbackData );
};

} // namespace smp::panel
