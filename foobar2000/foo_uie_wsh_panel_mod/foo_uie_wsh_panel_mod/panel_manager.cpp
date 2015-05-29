#include "stdafx.h"
#include "panel_manager.h"
#include "helpers.h"
#include "user_message.h"


/*static*/ panel_manager panel_manager::sm_instance;

namespace
{
	static service_factory_single_t<config_object_callback> g_config_object_callback;
	static playback_statistics_collector_factory_t<playback_stat_callback> g_stat_collector_callback;
	static play_callback_static_factory_t<my_play_callback > g_my_play_callback;
	static service_factory_single_t<my_playlist_callback> g_my_playlist_callback;
	static initquit_factory_t<nonautoregister_callbacks> g_nonautoregister_callbacks;
	static play_callback_static_factory_t<my_playback_queue_callback> g_my_playback_queue_callback;
	static library_callback_factory_t<my_library_callback> g_my_library_callback;
}

void panel_manager::send_msg_to_all(UINT p_msg, WPARAM p_wp, LPARAM p_lp)
{
	m_hwnds.for_each([p_msg, p_wp, p_lp](const HWND & hWnd) -> void 
	{ 
		SendMessage(hWnd, p_msg, p_wp, p_lp);
	});
}

void panel_manager::send_msg_to_others_pointer(HWND p_wnd_except, UINT p_msg, pfc::refcounted_object_root * p_param)
{
	t_size count = m_hwnds.get_count();

	if (count < 2 || !p_param)
		return;

	for (t_size i = 0; i < count - 1; ++i)
		p_param->refcount_add_ref();

	m_hwnds.for_each([p_msg, p_param, p_wnd_except] (const HWND & hWnd) -> void 
	{
		if (hWnd != p_wnd_except) 
		{
			SendMessage(hWnd, p_msg, reinterpret_cast<WPARAM>(p_param), 0); 
		}
	});
}

void panel_manager::post_msg_to_all(UINT p_msg, WPARAM p_wp, LPARAM p_lp)
{
	m_hwnds.for_each([p_msg, p_wp, p_lp] (const HWND & hWnd) -> void 
	{
		PostMessage(hWnd, p_msg, p_wp, p_lp); 
	});
}

void panel_manager::post_msg_to_all_pointer(UINT p_msg, pfc::refcounted_object_root * p_param)
{
	t_size count = m_hwnds.get_count();

	if (count < 1 || !p_param)
		return;

	for (t_size i = 0; i < count; ++i)
		p_param->refcount_add_ref();

	m_hwnds.for_each([p_msg, p_param] (const HWND & hWnd) -> void 
	{ 
		PostMessage(hWnd, p_msg, reinterpret_cast<WPARAM>(p_param), 0); 
	});
}

t_size config_object_callback::get_watched_object_count()
{
	return 3;
}

GUID config_object_callback::get_watched_object(t_size p_index)
{
	switch (p_index)
	{
	case 0:
		return standard_config_objects::bool_playlist_stop_after_current;

	case 1:
		return standard_config_objects::bool_cursor_follows_playback;

	case 2:
		return standard_config_objects::bool_playback_follows_cursor;
	}

	return pfc::guid_null;
}

void config_object_callback::on_watched_object_changed(const service_ptr_t<config_object> & p_object)
{
	GUID guid = p_object->get_guid();
	bool boolval = false;
	unsigned msg = 0;

	p_object->get_data_bool(boolval);

	if (guid == standard_config_objects::bool_playlist_stop_after_current)
		msg = CALLBACK_UWM_PLAYLIST_STOP_AFTER_CURRENT;
	else if (guid == standard_config_objects::bool_cursor_follows_playback)
		msg = CALLBACK_UWM_CURSOR_FOLLOW_PLAYBACK;
	else
		msg = CALLBACK_UWM_PLAYBACK_FOLLOW_CURSOR;

	panel_manager::instance().post_msg_to_all(msg, TO_VARIANT_BOOL(boolval));
}

void playback_stat_callback::on_item_played(metadb_handle_ptr p_item)
{
	simple_callback_data<metadb_handle_ptr> * on_item_played_data 
		= new simple_callback_data<metadb_handle_ptr>(p_item);

	panel_manager::instance().post_msg_to_all_pointer(CALLBACK_UWM_ON_ITEM_PLAYED, 
		on_item_played_data);
}

void nonautoregister_callbacks::on_changed_sorted(metadb_handle_list_cref p_items_sorted, bool p_fromhook)
{
	t_on_changed_sorted_data * on_changed_sorted_data = new t_on_changed_sorted_data(p_items_sorted, p_fromhook);

	panel_manager::instance().post_msg_to_all_pointer(CALLBACK_UWM_ON_CHANGED_SORTED, 
		on_changed_sorted_data);
}

void nonautoregister_callbacks::on_selection_changed(metadb_handle_list_cref p_selection)
{
	if (p_selection.get_count() > 0)
	{
		simple_callback_data<metadb_handle_ptr> * on_selection_changed_data 
			= new simple_callback_data<metadb_handle_ptr>(p_selection[0]);

		panel_manager::instance().post_msg_to_all_pointer(CALLBACK_UWM_ON_SELECTION_CHANGED, 
			on_selection_changed_data);
	}
	else
	{
		panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_SELECTION_CHANGED);
	}
}

void my_play_callback::on_playback_starting(play_control::t_track_command cmd, bool paused)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYBACK_STARTING, 
		(WPARAM)cmd, (LPARAM)paused);
}

void my_play_callback::on_playback_new_track(metadb_handle_ptr track)
{
	simple_callback_data<metadb_handle_ptr> * on_playback_new_track_data = new simple_callback_data<metadb_handle_ptr>(track);

	panel_manager::instance().post_msg_to_all_pointer(CALLBACK_UWM_ON_PLAYBACK_NEW_TRACK, 
		on_playback_new_track_data);
}

void my_play_callback::on_playback_stop(play_control::t_stop_reason reason)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYBACK_STOP, (WPARAM)reason);
}

void my_play_callback::on_playback_seek(double time)
{
	// sizeof(double) >= sizeof(WPARAM)
	simple_callback_data<double> * on_playback_seek_data = new simple_callback_data<double>(time);

	panel_manager::instance().post_msg_to_all_pointer(CALLBACK_UWM_ON_PLAYBACK_SEEK,
		on_playback_seek_data);
}

void my_play_callback::on_playback_pause(bool state)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYBACK_PAUSE, (WPARAM)state);
}

void my_play_callback::on_playback_edited(metadb_handle_ptr track)
{
	simple_callback_data<metadb_handle_ptr> * on_playback_edited_data = new simple_callback_data<metadb_handle_ptr>(track);

	panel_manager::instance().post_msg_to_all_pointer(CALLBACK_UWM_ON_PLAYBACK_EDITED, 
		on_playback_edited_data);
}

void my_play_callback::on_playback_dynamic_info(const file_info& info)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYBACK_DYNAMIC_INFO);
}

void my_play_callback::on_playback_dynamic_info_track(const file_info& info)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYBACK_DYNAMIC_INFO_TRACK);
}

void my_play_callback::on_playback_time(double time)
{
	// sizeof(double) >= sizeof(WPARAM)
	simple_callback_data<double> * on_playback_time_data = new simple_callback_data<double>(time);

	panel_manager::instance().post_msg_to_all_pointer(CALLBACK_UWM_ON_PLAYBACK_TIME,
		on_playback_time_data);
}

void my_play_callback::on_volume_change(float newval)
{
	// though sizeof(float) == sizeof(int), cast of IEEE754 is dangerous, always.
	simple_callback_data<float> * on_volume_change_data = new simple_callback_data<float>(newval);

	panel_manager::instance().post_msg_to_all_pointer(CALLBACK_UWM_ON_VOLUME_CHANGE,
		on_volume_change_data);
}

void my_playlist_callback::on_item_focus_change(t_size p_playlist,t_size p_from,t_size p_to)
{
	simple_callback_data_3<t_size, t_size, t_size> * on_item_focus_change_data = 
		new simple_callback_data_3<t_size, t_size, t_size>(p_playlist, p_from, p_to);
	panel_manager::instance().post_msg_to_all_pointer(CALLBACK_UWM_ON_ITEM_FOCUS_CHANGE, on_item_focus_change_data);
}

void my_playlist_callback::on_playback_order_changed(t_size p_new_index)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYBACK_ORDER_CHANGED, (WPARAM)p_new_index);
}

void my_playlist_callback::on_playlist_switch()
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYLIST_SWITCH);
}

void my_playlist_callback::on_playlists_changed()
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYLISTS_CHANGED);
}

void my_playlist_callback::on_items_added(t_size p_playlist,t_size p_start, const pfc::list_base_const_t<metadb_handle_ptr> & p_data,const bit_array & p_selection)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYLIST_ITEMS_ADDED, p_playlist);
}

void my_playlist_callback::on_items_reordered(t_size p_playlist,const t_size * p_order,t_size p_count)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYLIST_ITEMS_REORDERED, p_playlist);
}

void my_playlist_callback::on_items_removed(t_size p_playlist,const bit_array & p_mask,t_size p_old_count,t_size p_new_count)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYLIST_ITEMS_REMOVED, p_playlist, p_new_count);
}

void my_playlist_callback::on_items_selection_change(t_size p_playlist,const bit_array & p_affected,const bit_array & p_state)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYLIST_ITEMS_SELECTION_CHANGE);
}

void my_playlist_callback::on_item_ensure_visible(t_size p_playlist,t_size p_idx)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYLIST_ITEM_ENSURE_VISIBLE, p_playlist, p_idx);
}

void my_playlist_callback::on_playlist_activate(t_size p_old,t_size p_new)
{
	// redirect
	if (p_old != p_new) 
		on_playlist_switch();
}

void my_playlist_callback::on_playlist_created(t_size p_index,const char * p_name,t_size p_name_len)
{
	// redirect
	on_playlists_changed();
}

void my_playlist_callback::on_playlists_reorder(const t_size * p_order,t_size p_count)
{
	// redirect
	on_playlists_changed();
}

void my_playlist_callback::on_playlists_removed(const bit_array & p_mask,t_size p_old_count,t_size p_new_count)
{
	// redirect
	on_playlists_changed();
}

void my_playlist_callback::on_playlist_renamed(t_size p_index,const char * p_new_name,t_size p_new_name_len)
{
	// redirect
	on_playlists_changed();
}

void my_playback_queue_callback::on_changed(t_change_origin p_origin)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYBACK_QUEUE_CHANGED, (WPARAM)p_origin);
}

void my_library_callback::on_items_added(const pfc::list_base_const_t<metadb_handle_ptr> & p_data)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_LIBRARY_ITEMS_ADDED);
}

void my_library_callback::on_items_removed(const pfc::list_base_const_t<metadb_handle_ptr> & p_data)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_LIBRARY_ITEMS_REMOVED);
}

void my_library_callback::on_items_modified(const pfc::list_base_const_t<metadb_handle_ptr> & p_data)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_LIBRARY_ITEMS_CHANGED);
}
