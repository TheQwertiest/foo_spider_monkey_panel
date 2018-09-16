#include "stdafx.h"
#include "panel_manager.h"
#include "user_message.h"

namespace
{
	initquit_factory_t<my_initquit> g_my_initquit;
	library_callback_factory_t<my_library_callback> g_my_library_callback;
	play_callback_static_factory_t<my_play_callback_static> g_my_play_callback_static;
	play_callback_static_factory_t<my_playback_queue_callback> g_my_playback_queue_callback;
	playback_statistics_collector_factory_t<my_playback_statistics_collector> g_my_playback_statistics_collector;
	service_factory_single_t<my_config_object_notify> g_my_config_object_notify;
	service_factory_single_t<my_dsp_config_callback> g_my_dsp_config_callback;
	service_factory_single_t<my_metadb_io_callback> g_my_metadb_io_callback;
	service_factory_single_t<my_playlist_callback_static> g_my_playlist_callback_static;
}

panel_manager panel_manager::sm_instance;

panel_manager& panel_manager::instance()
{
	return sm_instance;
}

t_size panel_manager::get_count()
{
	return m_hwnds.size();
}

void panel_manager::add_window(HWND p_wnd)
{
    if ( m_hwnds.cend() == std::find( m_hwnds.cbegin(), m_hwnds.cend(), p_wnd ) )
    {
        m_hwnds.push_back( p_wnd );
    }
}

void panel_manager::post_msg_to_all(UINT p_msg)
{
	post_msg_to_all(p_msg, 0, 0);
}

void panel_manager::post_msg_to_all(UINT p_msg, WPARAM p_wp)
{
	post_msg_to_all(p_msg, p_wp, 0);
}

void panel_manager::post_msg_to_all(UINT p_msg, WPARAM p_wp, LPARAM p_lp)
{
    for (const auto& hWnd : m_hwnds )
    {
        PostMessage( hWnd, p_msg, p_wp, p_lp );
    }
}

void panel_manager::remove_window(HWND p_wnd)
{
    if ( auto it = std::find( m_hwnds.cbegin(), m_hwnds.cend(), p_wnd ); m_hwnds.cend() != it )
    {
        m_hwnds.erase( it );
    }
}

void panel_manager::send_msg_to_all(UINT p_msg, WPARAM p_wp, LPARAM p_lp)
{
    for ( const auto& hWnd : m_hwnds )
    {
        SendMessage( hWnd, p_msg, p_wp, p_lp );
    }
}

void panel_manager::send_msg_to_others(HWND p_wnd_except, UINT p_msg, WPARAM p_wp, LPARAM p_lp )
{
    if ( m_hwnds.size() < 2 )
    {
        return;
    }

    for ( const auto& hWnd : m_hwnds )
    {
        if ( hWnd != p_wnd_except )
        {
            SendMessage( hWnd, p_msg, p_wp, p_lp );
        }
    }	
}

void my_dsp_config_callback::on_core_settings_change(const dsp_chain_config& p_newdata)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_DSP_PRESET_CHANGED);
}

void my_initquit::on_selection_changed(metadb_handle_list_cref p_selection)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_SELECTION_CHANGED);
}

void my_initquit::on_init()
{
    if ( static_api_test_t<replaygain_manager_v2>() )
    {
        replaygain_manager_v2::get()->add_notify( this );
    }
    if ( static_api_test_t<output_manager_v2>() )
    {
        output_manager_v2::get()->addCallback( this );
    }
    ui_selection_manager_v2::get()->register_callback( this, 0 );
}

void my_initquit::on_quit()
{
    if ( static_api_test_t<replaygain_manager_v2>() )
    {
        replaygain_manager_v2::get()->remove_notify( this );
    }
    if ( static_api_test_t<output_manager_v2>() )
    {
        output_manager_v2::get()->removeCallback( this );
    }
    ui_selection_manager_v2::get()->unregister_callback( this );
}

void my_initquit::on_changed(t_replaygain_config const& cfg)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_REPLAYGAIN_MODE_CHANGED, (WPARAM)cfg.m_source_mode);
}

void my_initquit::outputConfigChanged()
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_OUTPUT_DEVICE_CHANGED);
}

void my_library_callback::on_items_added(metadb_handle_list_cref p_data)
{
	panel_manager::instance().post_msg_to_all_pointer( CALLBACK_UWM_ON_LIBRARY_ITEMS_ADDED, 
                                                       std::make_unique<metadb_callback_data>( p_data, false ));
}

void my_library_callback::on_items_modified(metadb_handle_list_cref p_data)
{
    panel_manager::instance().post_msg_to_all_pointer( CALLBACK_UWM_ON_LIBRARY_ITEMS_CHANGED,
                                                        std::make_unique<metadb_callback_data>( p_data, false ) );
}

void my_library_callback::on_items_removed(metadb_handle_list_cref p_data)
{
    panel_manager::instance().post_msg_to_all_pointer( CALLBACK_UWM_ON_LIBRARY_ITEMS_REMOVED,
                                                        std::make_unique<metadb_callback_data>( p_data, false ) );
}

void my_metadb_io_callback::on_changed_sorted(metadb_handle_list_cref p_items_sorted, bool p_fromhook)
{
    panel_manager::instance().post_msg_to_all_pointer( CALLBACK_UWM_ON_METADB_CHANGED,
                                                        std::make_unique<metadb_callback_data>( p_items_sorted, p_fromhook ) );
}

unsigned my_play_callback_static::get_flags()
{
	return flag_on_playback_all | flag_on_volume_change;
}

void my_play_callback_static::on_playback_dynamic_info(const file_info& info)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYBACK_DYNAMIC_INFO);
}

void my_play_callback_static::on_playback_dynamic_info_track(const file_info& info)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYBACK_DYNAMIC_INFO_TRACK);
}

void my_play_callback_static::on_playback_edited(metadb_handle_ptr track)
{	
    panel_manager::instance().post_msg_to_all_pointer( CALLBACK_UWM_ON_PLAYBACK_EDITED,
                                                       std::make_unique<metadb_handle_ptr>( track ) );
}

void my_play_callback_static::on_playback_new_track(metadb_handle_ptr track)
{
    panel_manager::instance().post_msg_to_all_pointer( CALLBACK_UWM_ON_PLAYBACK_NEW_TRACK,
                                                       std::make_unique<metadb_handle_ptr>( track ) );
}

void my_play_callback_static::on_playback_pause(bool state)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYBACK_PAUSE, (WPARAM)state);
}

void my_play_callback_static::on_playback_seek(double time)
{	
    panel_manager::instance().post_msg_to_all_pointer( CALLBACK_UWM_ON_PLAYBACK_SEEK,
                                                       std::make_unique<double>( time ) );
}

void my_play_callback_static::on_playback_starting(play_control::t_track_command cmd, bool paused)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYBACK_STARTING, (WPARAM)cmd, (LPARAM)paused);
}

void my_play_callback_static::on_playback_stop(play_control::t_stop_reason reason)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYBACK_STOP, (WPARAM)reason);
}

void my_play_callback_static::on_playback_time(double time)
{
    panel_manager::instance().post_msg_to_all_pointer( CALLBACK_UWM_ON_PLAYBACK_TIME,
                                                       std::make_unique<double>( time ) );
}

void my_play_callback_static::on_volume_change(float newval)
{	
    panel_manager::instance().post_msg_to_all_pointer( CALLBACK_UWM_ON_VOLUME_CHANGE,
                                                       std::make_unique<float>( newval ) );
}

void my_playback_queue_callback::on_changed(t_change_origin p_origin)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYBACK_QUEUE_CHANGED, (WPARAM)p_origin);
}

void my_playback_statistics_collector::on_item_played(metadb_handle_ptr p_item)
{
    panel_manager::instance().post_msg_to_all_pointer( CALLBACK_UWM_ON_ITEM_PLAYED,
                                                       std::make_unique<metadb_handle_ptr>( p_item ) );
}

GUID my_config_object_notify::get_watched_object(t_size p_index)
{
	switch (p_index)
	{
	case 0:
		return standard_config_objects::bool_playlist_stop_after_current;

	case 1:
		return standard_config_objects::bool_cursor_follows_playback;

	case 2:
		return standard_config_objects::bool_playback_follows_cursor;

	case 3:
		return standard_config_objects::bool_ui_always_on_top;
	}

	return pfc::guid_null;
}

t_size my_config_object_notify::get_watched_object_count()
{
	return 4;
}

void my_config_object_notify::on_watched_object_changed(const config_object::ptr& p_object)
{
	GUID guid = p_object->get_guid();
	bool boolval = false;
	unsigned msg = 0;

	p_object->get_data_bool(boolval);

	if (guid == standard_config_objects::bool_playlist_stop_after_current)
		msg = CALLBACK_UWM_ON_PLAYLIST_STOP_AFTER_CURRENT_CHANGED;
	else if (guid == standard_config_objects::bool_cursor_follows_playback)
		msg = CALLBACK_UWM_ON_CURSOR_FOLLOW_PLAYBACK_CHANGED;
	else if (guid == standard_config_objects::bool_playback_follows_cursor)
		msg = CALLBACK_UWM_ON_PLAYBACK_FOLLOW_CURSOR_CHANGED;
	else if (guid == standard_config_objects::bool_ui_always_on_top)
		msg = CALLBACK_UWM_ON_ALWAYS_ON_TOP_CHANGED;

	panel_manager::instance().post_msg_to_all(msg, TO_VARIANT_BOOL(boolval));
}

unsigned my_playlist_callback_static::get_flags()
{
	return flag_on_items_added | flag_on_items_reordered | flag_on_items_removed |
		flag_on_items_selection_change | flag_on_item_focus_change | flag_on_item_ensure_visible |
		flag_on_playlist_activate | flag_on_playlist_created | flag_on_playlists_reorder |
		flag_on_playlists_removed | flag_on_playlist_renamed | flag_on_playback_order_changed | flag_on_playlist_locked;
}

void my_playlist_callback_static::on_item_ensure_visible(t_size p_playlist, t_size p_idx)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYLIST_ITEM_ENSURE_VISIBLE, p_playlist, p_idx);
}

void my_playlist_callback_static::on_item_focus_change(t_size p_playlist, t_size p_from, t_size p_to)
{
    panel_manager::instance().post_msg_to_all_pointer( CALLBACK_UWM_ON_ITEM_FOCUS_CHANGE,
                                                       std::make_unique<std::tuple<t_size, t_size, t_size>>( p_playlist, p_from, p_to ) );
}

void my_playlist_callback_static::on_items_added(t_size p_playlist, t_size p_start, metadb_handle_list_cref p_data, const pfc::bit_array& p_selection)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYLIST_ITEMS_ADDED, p_playlist);
}

void my_playlist_callback_static::on_items_removed(t_size p_playlist, const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYLIST_ITEMS_REMOVED, p_playlist, p_new_count);
}

void my_playlist_callback_static::on_items_reordered(t_size p_playlist, const t_size* p_order, t_size p_count)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYLIST_ITEMS_REORDERED, p_playlist);
}

void my_playlist_callback_static::on_items_selection_change(t_size p_playlist, const pfc::bit_array& p_affected, const pfc::bit_array& p_state)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYLIST_ITEMS_SELECTION_CHANGE);
}

void my_playlist_callback_static::on_playback_order_changed(t_size p_new_index)
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYBACK_ORDER_CHANGED, (WPARAM)p_new_index);
}

void my_playlist_callback_static::on_playlist_activate(t_size p_old, t_size p_new)
{
	if (p_old != p_new)
	{
		on_playlist_switch();
	}
}

void my_playlist_callback_static::on_playlist_created(t_size p_index, const char* p_name, t_size p_name_len)
{
	on_playlists_changed();
}

void my_playlist_callback_static::on_playlist_locked(t_size p_playlist, bool p_locked)
{
	on_playlists_changed();
}

void my_playlist_callback_static::on_playlist_renamed(t_size p_index, const char* p_new_name, t_size p_new_name_len)
{
	on_playlists_changed();
}

void my_playlist_callback_static::on_playlists_removed(const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count)
{
	on_playlists_changed();
}

void my_playlist_callback_static::on_playlists_reorder(const t_size* p_order, t_size p_count)
{
	on_playlists_changed();
}

void my_playlist_callback_static::on_playlist_switch()
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYLIST_SWITCH);
}

void my_playlist_callback_static::on_playlists_changed()
{
	panel_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYLISTS_CHANGED);
}
