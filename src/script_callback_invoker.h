#pragma once

struct CallbackIds
{
	enum
	{
		on_always_on_top_changed,
		on_char,
		on_colours_changed,
		on_cursor_follow_playback_changed,
		on_drag_drop,
		on_drag_enter,
		on_drag_leave,
		on_drag_over,
		on_focus,
		on_font_changed,
		on_get_album_art_done,
		on_item_focus_change,
		on_item_played,
		on_key_down,
		on_key_up,
		on_library_items_added,
		on_library_items_changed,
		on_library_items_removed,
		on_load_image_done,
		on_main_menu,
		on_metadb_changed,
		on_mouse_lbtn_dblclk,
		on_mouse_lbtn_down,
		on_mouse_lbtn_up,
		on_mouse_leave,
		on_mouse_mbtn_dblclk,
		on_mouse_mbtn_down,
		on_mouse_mbtn_up,
		on_mouse_move,
		on_mouse_rbtn_dblclk,
		on_mouse_rbtn_down,
		on_mouse_rbtn_up,
		on_mouse_wheel,
		on_mouse_wheel_h,
		on_notify_data,
		on_paint,
		on_playback_dynamic_info,
		on_playback_dynamic_info_track,
		on_playback_edited,
		on_playback_follow_cursor_changed,
		on_playback_new_track,
		on_playback_order_changed,
		on_playback_pause,
		on_playback_queue_changed,
		on_playback_seek,
		on_playback_starting,
		on_playback_stop,
		on_playback_time,
		on_playlist_item_ensure_visible,
		on_playlist_items_added,
		on_playlist_items_removed,
		on_playlist_items_reordered,
		on_playlist_items_selection_change,
		on_playlist_stop_after_current_changed,
		on_playlist_switch,
		on_playlists_changed,
		on_script_unload,
		on_selection_changed,
		on_size,
		on_volume_change,
		end,
	};
};

class ScriptCallbackInvoker
{
public:
	ScriptCallbackInvoker();
	~ScriptCallbackInvoker();
	void init(IDispatch* pActiveScriptRoot);
	HRESULT invoke(int callbackId, VARIANTARG* argv = NULL, UINT argc = 0, VARIANT* ret = NULL);

	void reset()
	{
		m_callbackInvokerMap.remove_all();
	}

private:
	typedef pfc::map_t<int, int> CallbackInvokerMap;
	CallbackInvokerMap m_callbackInvokerMap;
	IDispatchPtr m_activeScriptRoot;
};
