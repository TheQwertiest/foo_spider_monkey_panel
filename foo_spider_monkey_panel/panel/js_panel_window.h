#pragma once

#include <config/parsed_panel_config.h>
#include <panel/event.h>
#include <panel/ievent_js_forwarder.h>
#include <panel/panel_info.h>
#include <panel/user_message.h>
#include <ui/ui_conf.h>

#include <queue>

namespace smp::com
{
class IDropTargetImpl;
}

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

class js_panel_window
    : public ui_helpers::container_window
{
public:
    js_panel_window( PanelType instanceType );
    virtual ~js_panel_window() = default;

public:
    // ui_helpers::container_window
    [[nodiscard]] class_data& get_class_data() const override;

    void ReloadScript();
    void LoadSettings( stream_reader& reader, t_size size, abort_callback& abort, bool reloadPanel = true );
    void SetSettings( const smp::config::ParsedPanelSettings& settings );
    bool UpdateSettings( const smp::config::PanelSettings& settings, bool reloadPanel = true );
    bool SaveSettings( stream_writer& writer, abort_callback& abort ) const;

    bool IsPanelIdOverridenByScript() const;

    void Fail( const qwr::u8string& errorText );

    void Repaint( bool force = false );
    void RepaintRect( const CRect& rc, bool force = false );
    /// @details Calls Repaint inside
    void RepaintBackground( const CRect& updateRc );

public: // accessors
    [[nodiscard]] qwr::u8string GetPanelId();
    [[nodiscard]] qwr::u8string GetPanelDescription( bool includeVersionAndAuthor = true );
    [[nodiscard]] HDC GetHDC() const;
    [[nodiscard]] HWND GetHWND() const;
    [[nodiscard]] POINT& MaxSize();
    [[nodiscard]] POINT& MinSize();
    [[nodiscard]] int GetHeight() const;
    [[nodiscard]] int GetWidth() const;
    [[nodiscard]] const config::ParsedPanelSettings& GetSettings() const;
    [[nodiscard]] config::PanelProperties& GetPanelProperties();

    [[nodiscard]] t_size& DlgCode();
    [[nodiscard]] PanelType GetPanelType() const;
    virtual DWORD GetColour( unsigned type, const GUID& guid ) = 0;
    virtual HFONT GetFont( unsigned type, const GUID& guid ) = 0;

    void SetScriptInfo( const qwr::u8string& scriptName, const qwr::u8string& scriptAuthor, const qwr::u8string& scriptVersion );
    void SetPanelName( const qwr::u8string& panelName );
    /// @throw qwr::QwrException
    void SetDragAndDropStatus( bool isEnabled );
    void SetCaptureFocusStatus( bool isEnabled );

protected:
    virtual void notify_size_limit_changed( LPARAM lp ) = 0;

    // ui_helpers::container_window
    LRESULT on_message( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp ) override;

    void EditScript();
    void ShowConfigure( HWND parent, ui::CDialogConf::Tab tab = ui::CDialogConf::Tab::def );

    void GenerateContextMenu( HMENU hMenu, int x, int y, uint32_t id_base );
    void ExecuteContextMenu( uint32_t id, uint32_t id_base );

private:
    bool ReloadSettings();
    bool LoadScript( bool isFirstLoad );
    void UnloadScript( bool force = false );
    void CreateDrawContext();
    void DeleteDrawContext();

public:
    void ExecuteJsTask( EventId id, IEvent_JsTask& task );

private: // callback handling
    std::optional<LRESULT> process_sync_messages( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp );
    std::optional<LRESULT> process_async_messages( UINT msg, WPARAM wp, LPARAM lp );
    std::optional<LRESULT> process_main_messages( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp );
    std::optional<LRESULT> process_window_messages( UINT msg, WPARAM wp, LPARAM lp );
    std::optional<LRESULT> process_internal_sync_messages( InternalSyncMessage msg, WPARAM wp, LPARAM lp );
    std::optional<LRESULT> process_internal_async_messages( InternalAsyncMessage msg, WPARAM wp, LPARAM lp );

    // Internal callbacks
    void OpenDefaultContextManu( int x, int y );
    void EraseBackground();
    void on_panel_create( HWND hWnd );
    void on_panel_destroy();
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
    void on_paint( HDC dc, const CRect& updateRc );
    void on_paint_error( HDC memdc );
    void on_paint_user( HDC memdc, const CRect& updateRc );
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

private:
    const PanelType panelType_;
    config::ParsedPanelSettings settings_ = config::ParsedPanelSettings::GetDefault();
    config::PanelProperties properties_;

    std::shared_ptr<mozjs::JsContainer> pJsContainer_;

    CWindow wnd_;
    HDC hDc_ = nullptr;

    uint32_t height_ = 0;     // Used externally as well
    uint32_t width_ = 0;      // Used externally as well
    CBitmap bmp_ = nullptr;   // used only internally
    CBitmap bmpBg_ = nullptr; // used only internally

    bool hasFailed_ = false;                   // // used only internally
    bool isBgRepaintNeeded_ = false;           // used only internally
    bool isPaintInProgress_ = false;           // used only internally
    bool isMouseTracked_ = false;              // used only internally
    ui_selection_holder::ptr selectionHolder_; // used only internally

    CComPtr<smp::com::IDropTargetImpl> dropTargetHandler_; // used only internally

    bool isPanelIdOverridenByScript_ = false; // used only internally

    size_t dlgCode_ = 0;                   // modified only from external
    POINT maxSize_ = { INT_MAX, INT_MAX }; // modified only from external
    POINT minSize_ = { 0, 0 };             // modified only from external
};

} // namespace smp::panel
