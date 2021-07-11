#pragma once

namespace smp::panel
{

class js_panel_window;

enum class EventId
{
    kFbAlwaysOnTopChanged,
    kFbCursorFollowPlaybackChanged,
    kFbDspPresetChanged,
    kFbItemFocusChange,
    kFbItemPlayed,
    kFbLibraryItemsAdded,
    kFbLibraryItemsChanged,
    kFbLibraryItemsRemoved,
    kFbMetadbChanged,
    kFbOutputDeviceChanged,
    kFbPlaybackDynamicInfo,
    kFbPlaybackDynamicInfoTrack,
    kFbPlaybackEdited,
    kFbPlaybackFollowCursorChanged,
    kFbPlaybackNewTrack,
    kFbPlaybackOrderChanged,
    kFbPlaybackPause,
    kFbPlaybackQueueChanged,
    kFbPlaybackSeek,
    kFbPlaybackStarting,
    kFbPlaybackStop,
    kFbPlaybackTime,
    kFbPlaylistItemEnsureVisible,
    kFbPlaylistItemsAdded,
    kFbPlaylistItemsRemoved,
    kFbPlaylistItemsReordered,
    kFbPlaylistItemsSelectionChange,
    kFbPlaylistStopAfterCurrentChanged,
    kFbPlaylistSwitch,
    kFbPlaylistsChanged,
    kFbReplaygainModeChanged,
    kFbSelectionChanged,
    kFbVolumeChange,
};

const std::unordered_map<EventId, qwr::u8string> kCallbackIdToName = {
    { EventId::kFbAlwaysOnTopChanged, "always_on_top_changed" },
    { EventId::kFbCursorFollowPlaybackChanged, "cursor_follow_playback_changed" },
    { EventId::kFbDspPresetChanged, "dsp_preset_changed" },
    { EventId::kFbItemFocusChange, "item_focus_change" },
    { EventId::kFbItemPlayed, "item_played" },
    { EventId::kFbLibraryItemsAdded, "library_items_added" },
    { EventId::kFbLibraryItemsChanged, "library_items_changed" },
    { EventId::kFbLibraryItemsRemoved, "library_items_removed" },
    { EventId::kFbMetadbChanged, "metadb_changed" },
    { EventId::kFbOutputDeviceChanged, "output_device_changed" },
    { EventId::kFbPlaybackDynamicInfo, "playback_dynamic_info" },
    { EventId::kFbPlaybackDynamicInfoTrack, "playback_dynamic_info_track" },
    { EventId::kFbPlaybackEdited, "playback_edited" },
    { EventId::kFbPlaybackFollowCursorChanged, "playback_follow_cursor_changed" },
    { EventId::kFbPlaybackNewTrack, "playback_new_track" },
    { EventId::kFbPlaybackOrderChanged, "playback_order_changed" },
    { EventId::kFbPlaybackPause, "playback_pause" },
    { EventId::kFbPlaybackQueueChanged, "playback_queue_changed" },
    { EventId::kFbPlaybackSeek, "playback_seek" },
    { EventId::kFbPlaybackStarting, "playback_starting" },
    { EventId::kFbPlaybackStop, "playback_stop" },
    { EventId::kFbPlaybackTime, "playback_time" },
    { EventId::kFbPlaylistItemEnsureVisible, "playlist_item_ensure_visible" },
    { EventId::kFbPlaylistItemsAdded, "playlist_items_added" },
    { EventId::kFbPlaylistItemsRemoved, "playlist_items_removed" },
    { EventId::kFbPlaylistItemsReordered, "playlist_items_reordered" },
    { EventId::kFbPlaylistItemsSelectionChange, "playlist_items_selection_change" },
    { EventId::kFbPlaylistStopAfterCurrentChanged, "playlist_stop_after_current_changed" },
    { EventId::kFbPlaylistSwitch, "playlist_switch" },
    { EventId::kFbPlaylistsChanged, "playlists_changed" },
    { EventId::kFbReplaygainModeChanged, "replaygain_mode_changed" },
    { EventId::kFbSelectionChanged, "selection_changed" },
    { EventId::kFbVolumeChange, "volume_change" },
};

enum class EventPriority
{
    kInputLow, // `input low` is used during drag-n-drop events or during shutting down
    kNormal,
    kInputHigh,
    kRedraw,
    kControl,
};

class Runnable
{
public:
    virtual ~Runnable() = default;
    virtual void Run( js_panel_window& panelWindow ) = 0;
};

} // namespace smp::panel
