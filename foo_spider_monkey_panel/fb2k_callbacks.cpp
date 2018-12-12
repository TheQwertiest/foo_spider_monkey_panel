#include <stdafx.h>

#include <message_manager.h>
#include <metadb_callback_data.h>

namespace
{

using namespace smp;
using namespace smp::panel;

class my_dsp_config_callback : public dsp_config_callback
{
public:
    void on_core_settings_change( const dsp_chain_config& p_newdata ) override;
};

class my_initquit
    : public initquit
    , public ui_selection_callback
    , public replaygain_core_settings_notify
    , public output_config_change_callback
{
public:
    void on_init() override;
    void on_quit() override;
    void on_changed( t_replaygain_config const& cfg ) override;
    void on_selection_changed( metadb_handle_list_cref p_selection ) override;
    void outputConfigChanged() override;
};

class my_library_callback : public library_callback
{
public:
    void on_items_added( metadb_handle_list_cref p_data ) override;
    void on_items_modified( metadb_handle_list_cref p_data ) override;
    void on_items_removed( metadb_handle_list_cref p_data ) override;
};

class my_metadb_io_callback : public metadb_io_callback
{
public:
    void on_changed_sorted( metadb_handle_list_cref p_items_sorted, bool p_fromhook ) override;
};

class my_play_callback_static : public play_callback_static
{
public:
    unsigned get_flags() override;
    void on_playback_dynamic_info( const file_info& info ) override;
    void on_playback_dynamic_info_track( const file_info& info ) override;
    void on_playback_edited( metadb_handle_ptr track ) override;
    void on_playback_new_track( metadb_handle_ptr track ) override;
    void on_playback_pause( bool state ) override;
    void on_playback_seek( double time ) override;
    void on_playback_starting( play_control::t_track_command cmd, bool paused ) override;
    void on_playback_stop( play_control::t_stop_reason reason ) override;
    void on_playback_time( double time ) override;
    void on_volume_change( float newval ) override;
};

class my_playback_queue_callback : public playback_queue_callback
{
public:
    void on_changed( t_change_origin p_origin ) override;
};

class my_playback_statistics_collector : public playback_statistics_collector
{
public:
    void on_item_played( metadb_handle_ptr p_item ) override;
};

class my_config_object_notify : public config_object_notify
{
public:
    GUID get_watched_object( t_size p_index ) override;
    t_size get_watched_object_count() override;
    void on_watched_object_changed( const config_object::ptr& p_object ) override;
};

class my_playlist_callback_static : public playlist_callback_static
{
public:
    unsigned get_flags() override;
    void on_default_format_changed() override;
    void on_item_ensure_visible( t_size p_playlist, t_size p_idx ) override;
    void on_item_focus_change( t_size p_playlist, t_size p_from, t_size p_to ) override;
    void on_items_added( t_size p_playlist, t_size p_start, metadb_handle_list_cref p_data, const pfc::bit_array& p_selection ) override;
    void on_items_removing( t_size p_playlist, const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count ) override;
    void on_items_removed( t_size p_playlist, const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count ) override;
    void on_items_reordered( t_size p_playlist, const t_size* p_order, t_size p_count ) override;
    void on_items_replaced( t_size p_playlist, const pfc::bit_array& p_mask, const pfc::list_base_const_t<t_on_items_replaced_entry>& p_data ) override;
    void on_items_selection_change( t_size p_playlist, const pfc::bit_array& p_affected, const pfc::bit_array& p_state ) override;
    void on_items_modified( t_size p_playlist, const pfc::bit_array& p_mask ) override;
    void on_items_modified_fromplayback( t_size p_playlist, const pfc::bit_array& p_mask, play_control::t_display_level p_level ) override;
    void on_playback_order_changed( t_size p_new_index ) override;
    void on_playlist_activate( t_size p_old, t_size p_new ) override;
    void on_playlist_created( t_size p_index, const char* p_name, t_size p_name_len ) override;
    void on_playlist_locked( t_size p_playlist, bool p_locked ) override;
    void on_playlist_renamed( t_size p_index, const char* p_new_name, t_size p_new_name_len ) override;
    void on_playlists_removing( const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count ) override;
    void on_playlists_removed( const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count ) override;
    void on_playlists_reorder( const t_size* p_order, t_size p_count ) override;

private:
    void on_playlist_switch();
    void on_playlists_changed();
};

} // namespace

namespace
{

void my_dsp_config_callback::on_core_settings_change( const dsp_chain_config& p_newdata )
{
    panel::message_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_dsp_preset_changed ) );
}

void my_initquit::on_selection_changed( metadb_handle_list_cref p_selection )
{
    panel::message_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_selection_changed ) );
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

void my_initquit::on_changed( t_replaygain_config const& cfg )
{
    panel::message_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_replaygain_mode_changed ), (WPARAM)cfg.m_source_mode );
}

void my_initquit::outputConfigChanged()
{
    panel::message_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_output_device_changed ) );
}

void my_library_callback::on_items_added( metadb_handle_list_cref p_data )
{
    panel::message_manager::instance().post_callback_msg_to_all( CallbackMessage::fb_library_items_added,
                                                                 std::make_unique<CallbackDataImpl<metadb_callback_data>>( metadb_callback_data( p_data, false ) ) );
}

void my_library_callback::on_items_modified( metadb_handle_list_cref p_data )
{
    panel::message_manager::instance().post_callback_msg_to_all( CallbackMessage::fb_library_items_changed,
                                                                 std::make_unique<CallbackDataImpl<metadb_callback_data>>( metadb_callback_data( p_data, false ) ) );
}

void my_library_callback::on_items_removed( metadb_handle_list_cref p_data )
{
    panel::message_manager::instance().post_callback_msg_to_all( CallbackMessage::fb_library_items_removed,
                                                                 std::make_unique<CallbackDataImpl<metadb_callback_data>>( metadb_callback_data( p_data, false ) ) );
}

void my_metadb_io_callback::on_changed_sorted( metadb_handle_list_cref p_items_sorted, bool p_fromhook )
{
    panel::message_manager::instance().post_callback_msg_to_all( CallbackMessage::fb_metadb_changed,
                                                                 std::make_unique<CallbackDataImpl<metadb_callback_data>>( metadb_callback_data( p_items_sorted, p_fromhook ) ) );
}

unsigned my_play_callback_static::get_flags()
{
    return flag_on_playback_all | flag_on_volume_change;
}

void my_play_callback_static::on_playback_dynamic_info( const file_info& info )
{
    panel::message_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_playback_dynamic_info ) );
}

void my_play_callback_static::on_playback_dynamic_info_track( const file_info& info )
{
    panel::message_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_playback_dynamic_info_track ) );
}

void my_play_callback_static::on_playback_edited( metadb_handle_ptr track )
{
    panel::message_manager::instance().post_callback_msg_to_all( CallbackMessage::fb_playback_edited,
                                                                 std::make_unique<CallbackDataImpl<metadb_handle_ptr>>( track ) );
}

void my_play_callback_static::on_playback_new_track( metadb_handle_ptr track )
{
    panel::message_manager::instance().post_callback_msg_to_all( CallbackMessage::fb_playback_new_track,
                                                                 std::make_unique<CallbackDataImpl<metadb_handle_ptr>>( track ) );
}

void my_play_callback_static::on_playback_pause( bool state )
{
    panel::message_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_playback_pause ), (WPARAM)state );
}

void my_play_callback_static::on_playback_seek( double time )
{
    panel::message_manager::instance().post_callback_msg_to_all( CallbackMessage::fb_playback_seek,
                                                                 std::make_unique<CallbackDataImpl<double>>( time ) );
}

void my_play_callback_static::on_playback_starting( play_control::t_track_command cmd, bool paused )
{
    panel::message_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_playback_starting ), (WPARAM)cmd, (LPARAM)paused );
}

void my_play_callback_static::on_playback_stop( play_control::t_stop_reason reason )
{
    panel::message_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_playback_stop ), (WPARAM)reason );
}

void my_play_callback_static::on_playback_time( double time )
{
    panel::message_manager::instance().post_callback_msg_to_all( CallbackMessage::fb_playback_time,
                                                                 std::make_unique<CallbackDataImpl<double>>( time ) );
}

void my_play_callback_static::on_volume_change( float newval )
{
    panel::message_manager::instance().post_callback_msg_to_all( CallbackMessage::fb_volume_change,
                                                                 std::make_unique<CallbackDataImpl<float>>( newval ) );
}

void my_playback_queue_callback::on_changed( t_change_origin p_origin )
{
    panel::message_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_playback_queue_changed ), (WPARAM)p_origin );
}

void my_playback_statistics_collector::on_item_played( metadb_handle_ptr p_item )
{
    panel::message_manager::instance().post_callback_msg_to_all( CallbackMessage::fb_item_played,
                                                                 std::make_unique<CallbackDataImpl<metadb_handle_ptr>>( p_item ) );
}

GUID my_config_object_notify::get_watched_object( t_size p_index )
{
    switch ( p_index )
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

void my_config_object_notify::on_watched_object_changed( const config_object::ptr& p_object )
{
    const GUID guid = p_object->get_guid();
    UINT msg;
    if ( guid == standard_config_objects::bool_playlist_stop_after_current )
    {
        msg = static_cast<UINT>( PlayerMessage::fb_playlist_stop_after_current_changed );
    }
    else if ( guid == standard_config_objects::bool_cursor_follows_playback )
    {
        msg = static_cast<UINT>( PlayerMessage::fb_cursor_follow_playback_changed );
    }
    else if ( guid == standard_config_objects::bool_playback_follows_cursor )
    {
        msg = static_cast<UINT>( PlayerMessage::fb_playback_follow_cursor_changed );
    }
    else if ( guid == standard_config_objects::bool_ui_always_on_top )
    {
        msg = static_cast<UINT>( PlayerMessage::fb_always_on_top_changed );
    }
    else
    {
        return;
    }

    bool boolval;
    p_object->get_data_bool( boolval );
    panel::message_manager::instance().post_msg_to_all( msg, boolval );
}

unsigned my_playlist_callback_static::get_flags()
{
    return flag_on_items_added | flag_on_items_reordered | flag_on_items_removed | flag_on_items_selection_change | flag_on_item_focus_change | flag_on_item_ensure_visible | flag_on_playlist_activate | flag_on_playlist_created | flag_on_playlists_reorder | flag_on_playlists_removed | flag_on_playlist_renamed | flag_on_playback_order_changed | flag_on_playlist_locked;
}

void my_playlist_callback_static::on_default_format_changed()
{
}

void my_playlist_callback_static::on_item_ensure_visible( t_size p_playlist, t_size p_idx )
{
    panel::message_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_playlist_item_ensure_visible ), p_playlist, p_idx );
}

void my_playlist_callback_static::on_item_focus_change( t_size p_playlist, t_size p_from, t_size p_to )
{
    panel::message_manager::instance().post_callback_msg_to_all( CallbackMessage::fb_item_focus_change,
                                                                 std::make_unique<CallbackDataImpl<t_size, t_size, t_size>>( p_playlist, p_from, p_to ) );
}

void my_playlist_callback_static::on_items_added( t_size p_playlist, t_size p_start, metadb_handle_list_cref p_data, const pfc::bit_array& p_selection )
{
    panel::message_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_playlist_items_added ), p_playlist );
}

void my_playlist_callback_static::on_items_removing( t_size p_playlist, const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count )
{
}

void my_playlist_callback_static::on_items_removed( t_size p_playlist, const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count )
{
    panel::message_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_playlist_items_removed ), p_playlist, p_new_count );
}

void my_playlist_callback_static::on_items_reordered( t_size p_playlist, const t_size* p_order, t_size p_count )
{
    panel::message_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_playlist_items_reordered ), p_playlist );
}

void my_playlist_callback_static::on_items_replaced( t_size p_playlist, const pfc::bit_array& p_mask, const pfc::list_base_const_t<t_on_items_replaced_entry>& p_data )
{
}

void my_playlist_callback_static::on_items_selection_change( t_size p_playlist, const pfc::bit_array& p_affected, const pfc::bit_array& p_state )
{
    panel::message_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_playlist_items_selection_change ) );
}

void my_playlist_callback_static::on_items_modified( t_size p_playlist, const pfc::bit_array& p_mask )
{
}

void my_playlist_callback_static::on_items_modified_fromplayback( t_size p_playlist, const pfc::bit_array& p_mask, play_control::t_display_level p_level )
{
}

void my_playlist_callback_static::on_playback_order_changed( t_size p_new_index )
{
    panel::message_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_playback_order_changed ), (WPARAM)p_new_index );
}

void my_playlist_callback_static::on_playlist_activate( t_size p_old, t_size p_new )
{
    if ( p_old != p_new )
    {
        on_playlist_switch();
    }
}

void my_playlist_callback_static::on_playlist_created( t_size p_index, const char* p_name, t_size p_name_len )
{
    on_playlists_changed();
}

void my_playlist_callback_static::on_playlist_locked( t_size p_playlist, bool p_locked )
{
    on_playlists_changed();
}

void my_playlist_callback_static::on_playlist_renamed( t_size p_index, const char* p_new_name, t_size p_new_name_len )
{
    on_playlists_changed();
}

void my_playlist_callback_static::on_playlists_removing( const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count )
{
}

void my_playlist_callback_static::on_playlists_removed( const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count )
{
    on_playlists_changed();
}

void my_playlist_callback_static::on_playlists_reorder( const t_size* p_order, t_size p_count )
{
    on_playlists_changed();
}

void my_playlist_callback_static::on_playlist_switch()
{
    panel::message_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_playlist_switch ) );
}

void my_playlist_callback_static::on_playlists_changed()
{
    panel::message_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_playlists_changed ) );
}

} // namespace

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

} // namespace
