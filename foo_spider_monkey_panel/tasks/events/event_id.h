#pragma once

namespace smp
{

enum class EventId
{
    // fb
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
    // input
    /// mouse
    //// drag-n-drop
    kMouseDragDrop,
    kMouseDragEnter,
    kMouseDragLeave,
    kMouseDragOver,
    /// main menu
    kStaticMainMenu,
    kDynamicMainMenu,
    // internal
    kInternalGetAlbumArtDone,
    kInternalGetAlbumArtPromiseDone,
    kInternalLoadImageDone,
    kInternalLoadImagePromiseDone,
    // ui
    kUiColoursChanged,
    kUiFontChanged,
    // window
    kWndRepaintBackground,
    // script
    kScriptEdit,
    kScriptReload,
    kScriptShowConfigure,
    kScriptShowConfigureLegacy,
    kScriptShowProperties,
    // custom
    kNotifyOthers,
    kTimer,
    // new
    // fb
    /// playback
    kNew_FbPlaybackDynamicInfo,
    kNew_FbPlaybackDynamicInfoTrack,
    kNew_FbPlaybackEdited,
    kNew_FbPlaybackNewTrack,
    kNew_FbPlaybackPlay,
    kNew_FbPlaybackPause,
    kNew_FbPlaybackSeek,
    kNew_FbPlaybackStarting,
    kNew_FbPlaybackStop,
    kNew_FbPlaybackTime,
    kNew_FbPlaybackVolumeChange,
    /// playlist
    kNew_FbPlaylistActivate,
    kNew_FbPlaylistCreated,
    kNew_FbPlaylistItemEnsureVisible,
    kNew_FbPlaylistItemFocusChange,
    kNew_FbPlaylistItemsAdded,
    kNew_FbPlaylistItemsRemoved,
    kNew_FbPlaylistItemsReordered,
    kNew_FbPlaylistItemsReplaced,
    kNew_FbPlaylistItemsSelectionChange,
    kNew_FbPlaylistLocked,
    /// TODO: handle in playback
    kNew_FbPlaylistPlaybackOrderChanged,
    kNew_FbPlaylistRenamed,
    kNew_FbPlaylistsRemoved,
    kNew_FbPlaylistsReorder,
    /// library
    kNew_FbLibraryItemsAdded,
    kNew_FbLibraryItemsModified,
    kNew_FbLibraryItemsRemoved,
    /// selection
    kNew_FbSelectionChange,
    // window
    /// graphics
    kNew_WndPaint,
    kNew_WndResize,
    /// control
    kNew_InputBlur,
    kNew_InputFocus,
    /// keyboard
    kNew_KeyboardKeyDown,
    kNew_KeyboardKeyUp,
    /// mouse
    //// buttons
    kNew_MouseButtonClick,
    kNew_MouseButtonAuxClick,
    kNew_MouseButtonDoubleClick,
    kNew_MouseButtonDoubleClickSystem,
    kNew_MouseButtonDown,
    kNew_MouseButtonUp,
    //// move
    kNew_MouseEnter,
    kNew_MouseLeave,
    kNew_MouseMove,
    // TODO: mouse enter
    //// context
    kNew_MouseContextMenu,
    //// wheel
    kNew_MouseWheel,
    // to be executed on js target
    kNew_JsTarget,
    // to be executed in js
    kNew_JsRunnable,
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
    // input
    /// mouse
    //// drag-n-drop
    { EventId::kMouseDragDrop, "UNUSED" },
    { EventId::kMouseDragEnter, "UNUSED" },
    { EventId::kMouseDragLeave, "UNUSED" },
    { EventId::kMouseDragOver, "UNUSED" },
    /// main menu
    { EventId::kStaticMainMenu, "main_menu" },
    { EventId::kDynamicMainMenu, "main_menu_dynamic" },
    // internal
    { EventId::kInternalGetAlbumArtDone, "get_album_art_done" },
    { EventId::kInternalLoadImageDone, "load_image_done" },
    // ui
    { EventId::kUiColoursChanged, "colours_changed" },
    { EventId::kUiFontChanged, "font_changed" },
    // custom
    { EventId::kNotifyOthers, "notify_data" },
};

} // namespace smp
