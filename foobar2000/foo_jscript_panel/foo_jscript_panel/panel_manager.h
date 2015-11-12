#pragma once


class panel_manager
{
public:
	typedef pfc::list_t<HWND> t_hwnd_list;

	panel_manager()
	{
	}

	static inline panel_manager & instance()
	{
		return sm_instance;
	}

	inline void add_window(HWND p_wnd)
	{
		if (m_hwnds.find_item(p_wnd) == pfc_infinite)
		{
			m_hwnds.add_item(p_wnd);
		}
	}

	inline void remove_window(HWND p_wnd)
	{
		m_hwnds.remove_item(p_wnd);
	}

	inline t_size get_count()
	{
		return m_hwnds.get_count();
	}

	void send_msg_to_all(UINT p_msg, WPARAM p_wp, LPARAM p_lp);
	void send_msg_to_others_pointer(HWND p_wnd_except, UINT p_msg, pfc::refcounted_object_root * p_param);
	//void post_msg_to_others_pointer(HWND p_wnd_except, UINT p_msg, pfc::refcounted_object_root * p_param);
	inline void post_msg_to_all(UINT p_msg) { post_msg_to_all(p_msg, 0, 0); }
	inline void post_msg_to_all(UINT p_msg, WPARAM p_wp) { post_msg_to_all(p_msg, p_wp, 0); }
	void post_msg_to_all(UINT p_msg, WPARAM p_wp, LPARAM p_lp);
	void post_msg_to_all_pointer(UINT p_msg, pfc::refcounted_object_root * p_param);

private:
	t_hwnd_list m_hwnds;
	static panel_manager sm_instance;

	PFC_CLASS_NOT_COPYABLE_EX(panel_manager)
};

class config_object_callback : public config_object_notify
{
public:
	virtual t_size get_watched_object_count();
	virtual GUID get_watched_object(t_size p_index);
	virtual void on_watched_object_changed(const service_ptr_t<config_object> & p_object);
};

template <typename T>
struct simple_callback_data : public pfc::refcounted_object_root
{
	T m_item;

	inline simple_callback_data(const T & p_item) : m_item(p_item) {}
};

template <typename T1, typename T2>
struct simple_callback_data_2 : public pfc::refcounted_object_root
{
	T1 m_item1;
	T2 m_item2;

	inline simple_callback_data_2(const T1 & p_item1, const T2 & p_item2) : m_item1(p_item1), m_item2(p_item2) {}
};

template <typename T1, typename T2, typename T3>
struct simple_callback_data_3 : public pfc::refcounted_object_root
{
	T1 m_item1;
	T2 m_item2;
	T3 m_item3;

	inline simple_callback_data_3(const T1 & p_item1, const T2 & p_item2, const T3 & p_item3)
		: m_item1(p_item1)
		, m_item2(p_item2)
		, m_item3(p_item3)
	{
	}
};

// Only used in message handler
template <class T>
class simple_callback_data_scope_releaser
{
private:
	T * m_data;

public:
	template <class TParam>
	inline simple_callback_data_scope_releaser(TParam p_data)
	{
		m_data = reinterpret_cast<T *>(p_data);
	}

	template <class TParam>
	inline simple_callback_data_scope_releaser(TParam * p_data)
	{
		m_data = reinterpret_cast<T *>(p_data);
	}

	inline ~simple_callback_data_scope_releaser()
	{
		m_data->refcount_release();
	}

	T * operator->()
	{
		return m_data;
	}
};

class playback_stat_callback : public playback_statistics_collector
{
public:
	virtual void on_item_played(metadb_handle_ptr p_item);
};

class nonautoregister_callbacks : public initquit, 
	public metadb_io_callback_dynamic, 
	public ui_selection_callback
{
public:
	struct t_on_changed_sorted_data : public pfc::refcounted_object_root
	{
		metadb_handle_list m_items_sorted;
		bool m_fromhook;

		t_on_changed_sorted_data(metadb_handle_list_cref p_items_sorted, bool p_fromhook) 
			: m_items_sorted(p_items_sorted)
			, m_fromhook(p_fromhook)
		{}
	};

	// initquit
	virtual void on_init()
	{
		static_api_ptr_t<metadb_io_v3>()->register_callback(this);
		static_api_ptr_t<ui_selection_manager_v2>()->register_callback(this, 0);
	}

	virtual void on_quit()
	{
		static_api_ptr_t<ui_selection_manager_v2>()->unregister_callback(this);
		static_api_ptr_t<metadb_io_v3>()->unregister_callback(this);
	}

	// metadb_io_callback_dynamic
	virtual void on_changed_sorted(metadb_handle_list_cref p_items_sorted, bool p_fromhook);

	// ui_selection_callback
	virtual void on_selection_changed(metadb_handle_list_cref p_selection);
};

class my_play_callback : public play_callback_static 
{
public:
	// flag_on_playback_all dosen't contain flag_on_volume_change!
	virtual unsigned get_flags() { return flag_on_playback_all | flag_on_volume_change; }

	virtual void on_playback_starting(play_control::t_track_command cmd, bool paused);
	virtual void on_playback_new_track(metadb_handle_ptr track);
	virtual void on_playback_stop(play_control::t_stop_reason reason);
	virtual void on_playback_seek(double time);
	virtual void on_playback_pause(bool state);
	virtual void on_playback_edited(metadb_handle_ptr track);
	virtual void on_playback_dynamic_info(const file_info& info);
	virtual void on_playback_dynamic_info_track(const file_info& info);
	virtual void on_playback_time(double time);
	virtual void on_volume_change(float newval);
};

class my_playlist_callback : public playlist_callback_static
{
public:
	virtual unsigned get_flags() 
	{
		return flag_on_items_added | flag_on_items_reordered | flag_on_items_removed | flag_on_item_focus_change | flag_on_items_selection_change |
			flag_on_playlist_activate | flag_on_playlist_created | flag_on_playlists_reorder |
			flag_on_playlists_removed | flag_on_playlist_renamed | flag_on_playback_order_changed ; 
	}

	virtual void on_items_added(t_size p_playlist,t_size p_start, const pfc::list_base_const_t<metadb_handle_ptr> & p_data,const bit_array & p_selection);
	virtual void on_items_reordered(t_size p_playlist,const t_size * p_order,t_size p_count);
	virtual void on_items_removing(t_size p_playlist,const bit_array & p_mask,t_size p_old_count,t_size p_new_count) {}
	virtual void on_items_removed(t_size p_playlist,const bit_array & p_mask,t_size p_old_count,t_size p_new_count);
	virtual void on_items_selection_change(t_size p_playlist,const bit_array & p_affected,const bit_array & p_state);
	virtual void on_item_focus_change(t_size p_playlist,t_size p_from,t_size p_to);
	virtual void on_items_modified(t_size p_playlist,const bit_array & p_mask) {}
	virtual void on_items_modified_fromplayback(t_size p_playlist,const bit_array & p_mask,play_control::t_display_level p_level) {}
	virtual void on_items_replaced(t_size p_playlist,const bit_array & p_mask,const pfc::list_base_const_t<t_on_items_replaced_entry> & p_data) {}
	virtual void on_item_ensure_visible(t_size p_playlist,t_size p_idx);
	virtual void on_playlist_activate(t_size p_old,t_size p_new);
	virtual void on_playlist_created(t_size p_index,const char * p_name,t_size p_name_len);
	virtual void on_playlists_reorder(const t_size * p_order,t_size p_count);
	virtual void on_playlists_removing(const bit_array & p_mask,t_size p_old_count,t_size p_new_count) {}
	virtual void on_playlists_removed(const bit_array & p_mask,t_size p_old_count,t_size p_new_count);
	virtual void on_playlist_renamed(t_size p_index,const char * p_new_name,t_size p_new_name_len);
	virtual void on_default_format_changed() {}
	virtual void on_playback_order_changed(t_size p_new_index);
	virtual void on_playlist_locked(t_size p_playlist,bool p_locked) {}
	
private:
	void on_playlist_switch();
	void on_playlists_changed();
};

class my_playback_queue_callback : public playback_queue_callback
{
public:
	void on_changed(t_change_origin p_origin);
};

class my_library_callback : public library_callback
{
public:
	virtual void on_items_added(const pfc::list_base_const_t<metadb_handle_ptr> & p_data);
	virtual void on_items_removed(const pfc::list_base_const_t<metadb_handle_ptr> & p_data);
	virtual void on_items_modified(const pfc::list_base_const_t<metadb_handle_ptr> & p_data);
};
