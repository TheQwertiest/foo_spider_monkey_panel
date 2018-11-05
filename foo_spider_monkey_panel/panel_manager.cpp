#include <stdafx.h>
#include "panel_manager.h"

#include <message_data_holder.h>

using namespace smp;

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

panel_manager panel_manager::sm_instance;

panel_manager& panel_manager::instance()
{
    return sm_instance;
}

t_size panel_manager::get_count()
{
    return m_hwnds.size();
}

void panel_manager::add_window( HWND p_wnd )
{
    if ( m_hwnds.cend() == std::find( m_hwnds.cbegin(), m_hwnds.cend(), p_wnd ) )
    {
        m_hwnds.push_back( p_wnd );
    }
}

void panel_manager::post_msg_to_all( UINT p_msg )
{
    post_msg_to_all( p_msg, 0, 0 );
}

void panel_manager::post_msg_to_all( UINT p_msg, WPARAM p_wp )
{
    post_msg_to_all( p_msg, p_wp, 0 );
}

void panel_manager::post_msg_to_all( UINT p_msg, WPARAM p_wp, LPARAM p_lp )
{
    for ( const auto& hWnd : m_hwnds )
    {
        PostMessage( hWnd, p_msg, p_wp, p_lp );
    }
}

void panel_manager::post_callback_msg( HWND p_wnd, smp::CallbackMessage p_msg, std::unique_ptr<smp::panel::CallbackData> data )
{
    auto& dataStorage = MessageDataHolder::GetInstance();

    std::shared_ptr<panel::CallbackData> sharedData( data.release() );
    dataStorage.StoreData( p_msg, std::vector<HWND>{p_wnd}, sharedData );

    BOOL bRet = PostMessage( p_wnd, static_cast<UINT>( p_msg ), 0, 0 );
    if ( !bRet )
    {
        dataStorage.FlushDataForHwnd( p_wnd, sharedData.get() );
    }
}

void panel_manager::post_callback_msg_to_all( CallbackMessage p_msg, std::unique_ptr<panel::CallbackData> data )
{
    if ( m_hwnds.empty() )
    {
        return;
    }

    auto& dataStorage = MessageDataHolder::GetInstance();

    std::shared_ptr<panel::CallbackData> sharedData( data.release() );
    dataStorage.StoreData( p_msg, m_hwnds, sharedData );

    for ( const auto& hWnd : m_hwnds )
    {
        BOOL bRet = PostMessage( hWnd, static_cast<UINT>( p_msg ), 0, 0 );
        if ( !bRet )
        {
            dataStorage.FlushDataForHwnd( hWnd, sharedData.get() );
        }
    }
}

void panel_manager::remove_window( HWND p_wnd )
{
    if ( auto it = std::find( m_hwnds.cbegin(), m_hwnds.cend(), p_wnd ); m_hwnds.cend() != it )
    {
        m_hwnds.erase( it );
    }
}

void panel_manager::send_msg_to_all( UINT p_msg, WPARAM p_wp, LPARAM p_lp )
{
    for ( const auto& hWnd : m_hwnds )
    {
        SendMessage( hWnd, p_msg, p_wp, p_lp );
    }
}

void panel_manager::send_msg_to_others( HWND p_wnd_except, UINT p_msg, WPARAM p_wp, LPARAM p_lp )
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

void my_dsp_config_callback::on_core_settings_change( const dsp_chain_config& p_newdata )
{
    panel_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_dsp_preset_changed ) );
}

void my_initquit::on_selection_changed( metadb_handle_list_cref p_selection )
{
    panel_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_selection_changed ) );
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
    panel_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_replaygain_mode_changed ), (WPARAM)cfg.m_source_mode );
}

void my_initquit::outputConfigChanged()
{
    panel_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_output_device_changed ) );
}

void my_library_callback::on_items_added( metadb_handle_list_cref p_data )
{
    panel_manager::instance().post_callback_msg_to_all( CallbackMessage::fb_library_items_added,
                                                        std::make_unique<smp::panel::CallbackDataImpl<metadb_callback_data>>( metadb_callback_data( p_data, false ) ) );
}

void my_library_callback::on_items_modified( metadb_handle_list_cref p_data )
{
    panel_manager::instance().post_callback_msg_to_all( CallbackMessage::fb_library_items_changed,
                                                        std::make_unique<smp::panel::CallbackDataImpl<metadb_callback_data>>( metadb_callback_data( p_data, false ) ) );
}

void my_library_callback::on_items_removed( metadb_handle_list_cref p_data )
{
    panel_manager::instance().post_callback_msg_to_all( CallbackMessage::fb_library_items_removed,
                                                        std::make_unique<smp::panel::CallbackDataImpl<metadb_callback_data>>( metadb_callback_data( p_data, false ) ) );
}

void my_metadb_io_callback::on_changed_sorted( metadb_handle_list_cref p_items_sorted, bool p_fromhook )
{
    panel_manager::instance().post_callback_msg_to_all( CallbackMessage::fb_metadb_changed,
                                                        std::make_unique<smp::panel::CallbackDataImpl<metadb_callback_data>>( metadb_callback_data( p_items_sorted, p_fromhook ) ) );
}

unsigned my_play_callback_static::get_flags()
{
    return flag_on_playback_all | flag_on_volume_change;
}

void my_play_callback_static::on_playback_dynamic_info( const file_info& info )
{
    panel_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_playback_dynamic_info ) );
}

void my_play_callback_static::on_playback_dynamic_info_track( const file_info& info )
{
    panel_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_playback_dynamic_info_track ) );
}

void my_play_callback_static::on_playback_edited( metadb_handle_ptr track )
{
    panel_manager::instance().post_callback_msg_to_all( CallbackMessage::fb_playback_edited,
                                                        std::make_unique<smp::panel::CallbackDataImpl<metadb_handle_ptr>>( track ) );
}

void my_play_callback_static::on_playback_new_track( metadb_handle_ptr track )
{
    panel_manager::instance().post_callback_msg_to_all( CallbackMessage::fb_playback_new_track,
                                                        std::make_unique<smp::panel::CallbackDataImpl<metadb_handle_ptr>>( track ) );
}

void my_play_callback_static::on_playback_pause( bool state )
{
    panel_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_playback_pause ), (WPARAM)state );
}

void my_play_callback_static::on_playback_seek( double time )
{
    panel_manager::instance().post_callback_msg_to_all( CallbackMessage::fb_playback_seek,
                                                        std::make_unique<smp::panel::CallbackDataImpl<double>>( time ) );
}

void my_play_callback_static::on_playback_starting( play_control::t_track_command cmd, bool paused )
{
    panel_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_playback_starting ), (WPARAM)cmd, (LPARAM)paused );
}

void my_play_callback_static::on_playback_stop( play_control::t_stop_reason reason )
{
    panel_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_playback_stop ), (WPARAM)reason );
}

void my_play_callback_static::on_playback_time( double time )
{
    panel_manager::instance().post_callback_msg_to_all( CallbackMessage::fb_playback_time,
                                                        std::make_unique<smp::panel::CallbackDataImpl<double>>( time ) );
}

void my_play_callback_static::on_volume_change( float newval )
{
    panel_manager::instance().post_callback_msg_to_all( CallbackMessage::fb_volume_change,
                                                        std::make_unique<smp::panel::CallbackDataImpl<float>>( newval ) );
}

void my_playback_queue_callback::on_changed( t_change_origin p_origin )
{
    panel_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_playback_queue_changed ), (WPARAM)p_origin );
}

void my_playback_statistics_collector::on_item_played( metadb_handle_ptr p_item )
{
    panel_manager::instance().post_callback_msg_to_all( CallbackMessage::fb_item_played,
                                                        std::make_unique<smp::panel::CallbackDataImpl<metadb_handle_ptr>>( p_item ) );
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
    GUID guid = p_object->get_guid();
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
    panel_manager::instance().post_msg_to_all( msg, boolval );
}

unsigned my_playlist_callback_static::get_flags()
{
    return flag_on_items_added | flag_on_items_reordered | flag_on_items_removed | flag_on_items_selection_change | flag_on_item_focus_change | flag_on_item_ensure_visible | flag_on_playlist_activate | flag_on_playlist_created | flag_on_playlists_reorder | flag_on_playlists_removed | flag_on_playlist_renamed | flag_on_playback_order_changed | flag_on_playlist_locked;
}

void my_playlist_callback_static::on_item_ensure_visible( t_size p_playlist, t_size p_idx )
{
    panel_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_playlist_item_ensure_visible ), p_playlist, p_idx );
}

void my_playlist_callback_static::on_item_focus_change( t_size p_playlist, t_size p_from, t_size p_to )
{
    panel_manager::instance().post_callback_msg_to_all( CallbackMessage::fb_item_focus_change,
                                                        std::make_unique<smp::panel::CallbackDataImpl<t_size, t_size, t_size>>( p_playlist, p_from, p_to ) );
}

void my_playlist_callback_static::on_items_added( t_size p_playlist, t_size p_start, metadb_handle_list_cref p_data, const pfc::bit_array& p_selection )
{
    panel_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_playlist_items_added ), p_playlist );
}

void my_playlist_callback_static::on_items_removed( t_size p_playlist, const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count )
{
    panel_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_playlist_items_removed ), p_playlist, p_new_count );
}

void my_playlist_callback_static::on_items_reordered( t_size p_playlist, const t_size* p_order, t_size p_count )
{
    panel_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_playlist_items_reordered ), p_playlist );
}

void my_playlist_callback_static::on_items_selection_change( t_size p_playlist, const pfc::bit_array& p_affected, const pfc::bit_array& p_state )
{
    panel_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_playlist_items_selection_change ) );
}

void my_playlist_callback_static::on_playback_order_changed( t_size p_new_index )
{
    panel_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_playback_order_changed ), (WPARAM)p_new_index );
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
    panel_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_playlist_switch ) );
}

void my_playlist_callback_static::on_playlists_changed()
{
    panel_manager::instance().post_msg_to_all( static_cast<UINT>( PlayerMessage::fb_playlists_changed ) );
}
