#pragma once

namespace smp::panel
{

class js_panel_window;

}

namespace smp
{

class Event_Mouse;
class Event_Focus;

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
    /// control
    kInputFocus,
    /// keyboard
    kKeyboardChar,
    kKeyboardKeyDown,
    kKeyboardKeyUp,
    /// mouse
    //// buttons
    kMouseLeftButtonDoubleClick,
    kMouseLeftButtonDown,
    kMouseLeftButtonUp,
    kMouseMiddleButtonDoubleClick,
    kMouseMiddleButtonDown,
    kMouseMiddleButtonUp,
    kMouseRightButtonDoubleClick,
    kMouseRightButtonDown,
    kMouseRightButtonUp,
    //// wheel
    kMouseHorizontalWheel,
    kMouseVerticalWheel,
    //// move
    kMouseLeave,
    kMouseMove,
    //// context
    kMouseContextMenu,
    // internal
    kInternalGetAlbumArtDone,
    kInternalGetAlbumArtPromiseDone,
    kInternalLoadImageDone,
    kInternalLoadImagePromiseDone,
    kInternalMainMenu,
    kInternalMainMenuDynamic,
    // ui
    kUiColoursChanged,
    kUiFontChanged,
    // window
    kWndFocus,
    kWndPaint,
    kWndResize,
    // script
    kScriptEdit,
    kScriptReload,
    kScriptShowConfigure,
    kScriptShowConfigureLegacy,
    kScriptShowProperties,
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
    /// control
    { EventId::kInputFocus, "focus" },
    /// keyboard
    { EventId::kKeyboardChar, "char" },
    { EventId::kKeyboardKeyDown, "key_down" },
    { EventId::kKeyboardKeyUp, "key_up" },
    /// mouse
    //// buttons
    { EventId::kMouseLeftButtonDoubleClick, "mouse_lbtn_dblclk" },
    { EventId::kMouseLeftButtonDown, "mouse_lbtn_down" },
    { EventId::kMouseLeftButtonUp, "mouse_lbtn_up" },
    { EventId::kMouseMiddleButtonDoubleClick, "mouse_mbtn_dblclk" },
    { EventId::kMouseMiddleButtonDown, "mouse_mbtn_down" },
    { EventId::kMouseMiddleButtonUp, "mouse_mbtn_up" },
    { EventId::kMouseRightButtonDoubleClick, "mouse_rbtn_dblclk" },
    { EventId::kMouseRightButtonDown, "mouse_rbtn_down" },
    { EventId::kMouseRightButtonUp, "mouse_rbtn_up" },
    //// wheel
    { EventId::kMouseHorizontalWheel, "mouse_wheel_h" },
    { EventId::kMouseVerticalWheel, "mouse_wheel" },
    //// move
    { EventId::kMouseLeave, "mouse_leave" },
    { EventId::kMouseMove, "mouse_move" },
    //// context
    { EventId::kMouseContextMenu, "TODO" },
    // internal
    { EventId::kInternalGetAlbumArtDone, "get_album_art_done" },
    { EventId::kInternalLoadImageDone, "load_image_done" },
    { EventId::kInternalMainMenu, "main_menu" },
    { EventId::kInternalMainMenuDynamic, "main_menu_dynamic" },
    // ui
    { EventId::kUiColoursChanged, "colours_changed" },
    { EventId::kUiFontChanged, "font_changed" },
    // window
    { EventId::kWndPaint, "TODO" },
    { EventId::kWndResize, "TODO" },
};

enum class EventPriority
{
    kInputLow, // `input low` is used during drag-n-drop events or during shutting down
    kNormal,
    kInputHigh,
    kRedraw,
    kResize,
    kControl,
};

class Runnable
{
public:
    virtual ~Runnable() = default;
    virtual void Run( panel::js_panel_window& panelWindow ) = 0;
};

class EventBase
{
public:
    virtual ~EventBase() = default;

    virtual Event_Mouse* AsMouseEvent()
    {
        return nullptr;
    };
    virtual Event_Focus* AsFocusEvent()
    {
        return nullptr;
    };
};

} // namespace smp
