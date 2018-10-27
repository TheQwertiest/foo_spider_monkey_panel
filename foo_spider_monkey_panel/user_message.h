#pragma once

namespace smp
{

enum class CallbackMessage: UINT
{
    firstMessage = WM_USER + 100,
    fb_item_focus_change = firstMessage,
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
    lastMessage = fb_volume_change,
};

enum class PlayerMessage : UINT
{
    firstMessage = static_cast<int>(CallbackMessage::lastMessage) + 1,
    fb_always_on_top_changed = firstMessage,
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
    fb_playlist_items_removed,
    fb_playlist_items_reordered,
    fb_playlist_items_selection_change,
    fb_playlist_stop_after_current_changed,
    fb_playlist_switch,
    fb_playlists_changed,
    fb_replaygain_mode_changed,
    fb_selection_changed,
    ui_colours_changed,
    ui_font_changed,
    wnd_drag_drop,
    wnd_drag_enter,
    wnd_drag_leave,
    wnd_drag_over,
    lastMessage = wnd_drag_over,
};

enum class InternalMessage: UINT
{
    firstMessage = static_cast<int>(PlayerMessage::lastMessage) + 1,
    main_menu_item,
    get_album_art_done,
    load_image_done,
    notify_data,
    refresh_bg,
    reload_script,
    script_error,
    terminate_script,
    show_configure,
    show_properties,
    update_size,
    size_limit_changed,
    timer_proc,
    lastMessage = timer_proc,
};

enum class MiscMessage : UINT
{
    find_text_changed = static_cast<int>( InternalMessage::lastMessage ) + 1,
    heartbeat,
    key_down,
};

} // namespace smp
