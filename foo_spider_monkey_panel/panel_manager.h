#pragma once
#include "helpers.h"

namespace smp::panel
{

// TODO: try to make optimizations for integral types

template <class T>
using CallbackData = std::shared_ptr<T>;

template <class T>
class ScopedCallbackData
{
public:
    template <class TParam>
    ScopedCallbackData( TParam p_data )
        : m_data( reinterpret_cast<CallbackData<T>*>(p_data) )
    {
    }

    T* operator->()
    {
        return &get();
    }

    const T* operator->() const
    {
        return &get();
    }

    T& operator*()
    {
        return get();
    }

    const T& operator*() const
    {
        return get();
    }

    T& get()
    {
        return **m_data;
    }

    const T& get() const
    {
        return **m_data;
    }

private:
    ScopedCallbackData( const ScopedCallbackData & ) = delete;
    ScopedCallbackData operator=( const ScopedCallbackData & ) = delete;

private:
    std::unique_ptr<CallbackData<T>> m_data;
};

}

// TODO: consider removing fromhook
struct metadb_callback_data
{
	metadb_handle_list m_items;
	bool m_fromhook;

	metadb_callback_data(metadb_handle_list_cref p_items, bool p_fromhook) 
        : m_items(p_items)
        , m_fromhook(p_fromhook)
	{
	}
};

class panel_manager
{
public:
    panel_manager() = default;

	static panel_manager& instance();
	t_size get_count();
	void add_window(HWND p_wnd);
	void post_msg_to_all(UINT p_msg);
	void post_msg_to_all(UINT p_msg, WPARAM p_wp);
	void post_msg_to_all(UINT p_msg, WPARAM p_wp, LPARAM p_lp);
    template<typename T>
    void post_msg_to_all_pointer( UINT p_msg, std::unique_ptr<T> data )
    {
        if ( m_hwnds.empty() )
        {
            return;
        }

        std::shared_ptr<T> sharedData( std::move( data ) );
        for ( const auto& hWnd : m_hwnds )
        {
            PostMessage( hWnd, p_msg,
                         // ya, rly
                         reinterpret_cast<WPARAM>(new smp::panel::CallbackData<T>( sharedData )),
                         0 );
        }
    }
	void remove_window(HWND p_wnd);
	void send_msg_to_all(UINT p_msg, WPARAM p_wp, LPARAM p_lp);
	void send_msg_to_others(HWND p_wnd_except, UINT p_msg, WPARAM p_wp, LPARAM p_lp );
    template<typename T>
    void send_msg_to_others_pointer( HWND p_wnd_except, UINT p_msg, std::unique_ptr<T> data )
    {
        if ( m_hwnds.empty() )
        {
            return;
        }

        // Don't really need this (SendMessage is synchronous),
        // but this way we can use the same interface everywhere
        std::shared_ptr<T> sharedData( std::move(data) );  
        for ( const auto& hWnd : m_hwnds )
        {
            if ( hWnd != p_wnd_except )
            {
                SendMessage( hWnd, p_msg,
                             // ya, rly
                             reinterpret_cast<WPARAM>(new smp::panel::CallbackData<T>( sharedData )),
                             0 );
            }
            
        }
    }

private:
	std::vector<HWND> m_hwnds;
	static panel_manager sm_instance;

	PFC_CLASS_NOT_COPYABLE_EX(panel_manager)
};

class my_dsp_config_callback : public dsp_config_callback
{
public:
	virtual void on_core_settings_change(const dsp_chain_config& p_newdata);
};

class my_initquit 
    : public initquit
    , public ui_selection_callback
    , public replaygain_core_settings_notify
    , public output_config_change_callback
{
public:
    virtual void on_init();
    virtual void on_quit();
	virtual void on_changed(t_replaygain_config const& cfg);
	virtual void on_selection_changed(metadb_handle_list_cref p_selection);
	virtual void outputConfigChanged();
};

class my_library_callback : public library_callback
{
public:
	virtual void on_items_added(metadb_handle_list_cref p_data);
	virtual void on_items_modified(metadb_handle_list_cref p_data);
	virtual void on_items_removed(metadb_handle_list_cref p_data);
};

class my_metadb_io_callback : public metadb_io_callback
{
public:
	virtual void on_changed_sorted(metadb_handle_list_cref p_items_sorted, bool p_fromhook);
};

class my_play_callback_static : public play_callback_static
{
public:
	virtual unsigned get_flags();
	virtual void on_playback_dynamic_info(const file_info& info);
	virtual void on_playback_dynamic_info_track(const file_info& info);
	virtual void on_playback_edited(metadb_handle_ptr track);
	virtual void on_playback_new_track(metadb_handle_ptr track);
	virtual void on_playback_pause(bool state);
	virtual void on_playback_seek(double time);
	virtual void on_playback_starting(play_control::t_track_command cmd, bool paused);
	virtual void on_playback_stop(play_control::t_stop_reason reason);
	virtual void on_playback_time(double time);
	virtual void on_volume_change(float newval);
};

class my_playback_queue_callback : public playback_queue_callback
{
public:
	void on_changed(t_change_origin p_origin);
};

class my_playback_statistics_collector : public playback_statistics_collector
{
public:
	virtual void on_item_played(metadb_handle_ptr p_item);
};

class my_config_object_notify : public config_object_notify
{
public:
	virtual GUID get_watched_object(t_size p_index);
	virtual t_size get_watched_object_count();
	virtual void on_watched_object_changed(const config_object::ptr& p_object);
};

class my_playlist_callback_static : public playlist_callback_static
{
public:
	virtual void on_default_format_changed() {}
	virtual void on_items_modified(t_size p_playlist, const pfc::bit_array& p_mask) {}
	virtual void on_items_modified_fromplayback(t_size p_playlist, const pfc::bit_array& p_mask, play_control::t_display_level p_level) {}
	virtual void on_items_removing(t_size p_playlist, const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count) {}
	virtual void on_items_replaced(t_size p_playlist, const pfc::bit_array& p_mask, const pfc::list_base_const_t<t_on_items_replaced_entry>& p_data) {}
	virtual void on_playlists_removing(const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count) {}

	virtual unsigned get_flags();
	virtual void on_item_ensure_visible(t_size p_playlist, t_size p_idx);
	virtual void on_item_focus_change(t_size p_playlist, t_size p_from, t_size p_to);
	virtual void on_items_added(t_size p_playlist, t_size p_start, metadb_handle_list_cref p_data, const pfc::bit_array& p_selection);
	virtual void on_items_removed(t_size p_playlist, const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count);
	virtual void on_items_reordered(t_size p_playlist, const t_size* p_order, t_size p_count);
	virtual void on_items_selection_change(t_size p_playlist, const pfc::bit_array& p_affected, const pfc::bit_array& p_state);
	virtual void on_playback_order_changed(t_size p_new_index);
	virtual void on_playlist_activate(t_size p_old, t_size p_new);
	virtual void on_playlist_created(t_size p_index, const char* p_name, t_size p_name_len);
	virtual void on_playlist_locked(t_size p_playlist, bool p_locked);
	virtual void on_playlist_renamed(t_size p_index, const char* p_new_name, t_size p_new_name_len);
	virtual void on_playlists_removed(const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count);
	virtual void on_playlists_reorder(const t_size* p_order, t_size p_count);

private:
	void on_playlist_switch();
	void on_playlists_changed();
};
