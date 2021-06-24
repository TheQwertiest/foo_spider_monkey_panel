#pragma once

namespace smp
{

/// @details These messages are asynchronous
enum class CallbackMessage : UINT
{
    first_message = WM_USER + 100,
    fb_item_focus_change = first_message,
    fb_item_played,
    fb_library_items_added,
    fb_library_items_changed,
    fb_library_items_removed,
    fb_metadb_changed,
    fb_playback_edited,
    fb_playback_new_track,
    fb_playback_seek,
    fb_playback_time,
    fb_volume_change,
    internal_get_album_art_done,
    internal_get_album_art_promise_done,
    internal_load_image_done,
    internal_load_image_promise_done,
    last_message = internal_load_image_promise_done,
};

/// @details These messages are asynchronous
enum class PlayerMessage : UINT
{
    first_message = static_cast<int>( CallbackMessage::last_message ) + 1,
    fb_always_on_top_changed = first_message,
    fb_cursor_follow_playback_changed,
    fb_dsp_preset_changed,
    fb_output_device_changed,
    fb_playback_dynamic_info,
    fb_playback_dynamic_info_track,
    fb_playback_follow_cursor_changed,
    fb_playback_order_changed,
    fb_playback_pause,
    fb_playback_queue_changed,
    fb_playback_starting,
    fb_playback_stop,
    fb_playlist_item_ensure_visible,
    fb_playlist_items_added,
    fb_playlist_items_reordered,
    fb_playlist_items_removed,
    fb_playlist_items_selection_change,
    fb_playlist_stop_after_current_changed,
    fb_playlist_switch,
    fb_playlists_changed,
    fb_replaygain_mode_changed,
    fb_selection_changed,
    ui_colours_changed,
    ui_font_changed,
    last_message = ui_font_changed,
};

/// @details These messages are asynchronous
enum class InternalAsyncMessage : UINT
{
    first_message = static_cast<int>( PlayerMessage::last_message ) + 1,
    edit_script = first_message,
    main_menu_item,
    refresh_bg,
    reload_script,
    show_configure_legacy,
    show_configure,
    show_properties,
    last_message = show_properties,
};

/// @details These messages are synchronous
enum class InternalSyncMessage : UINT
{
    first_message = static_cast<int>( InternalAsyncMessage::last_message ) + 1,
    notify_data = first_message,
    script_fail,
    terminate_script,
    timer_proc,
    ui_script_editor_saved,
    update_size_on_reload,
    wnd_drag_drop,
    wnd_drag_enter,
    wnd_drag_leave,
    wnd_drag_over,
    last_message = wnd_drag_over,
};

/// @brief Message definitions that are not handled by the main panel window
enum class MiscMessage : UINT
{
    heartbeat = static_cast<int>( InternalSyncMessage::last_message ) + 1,
    key_down,
    run_task_async,
    size_limit_changed
};

template <typename T>
bool IsInEnumRange( UINT value )
{
    return ( value >= static_cast<UINT>( T::first_message )
             && value <= static_cast<UINT>( T::last_message ) );
}

} // namespace smp
