#include <stdafx.h>

#include <events/event_dispatcher.h>
#include <events/event_js_callback.h>
#include <fb2k/playlist_lock.h>

#include <qwr/error_popup.h>

namespace
{

using namespace smp;
using namespace smp::panel;

class InitStageCallback : public init_stage_callback
{
    void on_init_stage( t_uint32 stage ) override;
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

class my_dsp_config_callback : public dsp_config_callback
{
public:
    void on_core_settings_change( const dsp_chain_config& p_newdata ) override;
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
    my_config_object_notify();

    GUID get_watched_object( t_size p_index ) override;
    t_size get_watched_object_count() override;
    void on_watched_object_changed( const config_object::ptr& p_object ) override;

private:
    const std::array<std::pair<GUID, EventId>, 4> watchedObjects_;
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

void InitStageCallback::on_init_stage( t_uint32 stage )
{
    if ( stage == init_stages::before_ui_init )
    { // SMP is invoked during ui initialization, hence we must init locks before that,
        // so that scripts would receive correct lock states
        try
        {
            smp::PlaylistLockManager::Get().InitializeLocks();
        }
        catch ( const qwr::QwrException& e )
        {
            qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, fmt::format( "Failed to initialize playlist locks: {}", e.what() ) );
        }
    }
}

void my_initquit::on_selection_changed( metadb_handle_list_cref )
{
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kFbSelectionChanged ) );
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

void my_initquit::on_changed( const t_replaygain_config& cfg )
{
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kFbReplaygainModeChanged, static_cast<uint32_t>( cfg.m_source_mode ) ) );
}

void my_initquit::outputConfigChanged()
{
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kFbOutputDeviceChanged ) );
}

void my_dsp_config_callback::on_core_settings_change( const dsp_chain_config& )
{
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kFbDspPresetChanged ) );
}

void my_library_callback::on_items_added( metadb_handle_list_cref p_data )
{
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kFbLibraryItemsAdded, std::make_shared<metadb_handle_list>( std::move( p_data ) ) ) );
}

void my_library_callback::on_items_modified( metadb_handle_list_cref p_data )
{
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kFbLibraryItemsChanged, std::make_shared<metadb_handle_list>( std::move( p_data ) ) ) );
}

void my_library_callback::on_items_removed( metadb_handle_list_cref p_data )
{
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kFbLibraryItemsRemoved, std::make_shared<metadb_handle_list>( std::move( p_data ) ) ) );
}

void my_metadb_io_callback::on_changed_sorted( metadb_handle_list_cref p_items_sorted, bool p_fromhook )
{
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kFbMetadbChanged, std::make_shared<metadb_handle_list>( std::move( p_items_sorted ) ), p_fromhook ) );
}

unsigned my_play_callback_static::get_flags()
{
    return flag_on_playback_all | flag_on_volume_change;
}

void my_play_callback_static::on_playback_dynamic_info( const file_info& )
{
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kFbPlaybackDynamicInfo ) );
}

void my_play_callback_static::on_playback_dynamic_info_track( const file_info& )
{
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kFbPlaybackDynamicInfoTrack ) );
}

void my_play_callback_static::on_playback_edited( metadb_handle_ptr track )
{
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kFbPlaybackEdited, track ) );
}

void my_play_callback_static::on_playback_new_track( metadb_handle_ptr track )
{
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kFbPlaybackNewTrack, track ) );
}

void my_play_callback_static::on_playback_pause( bool state )
{
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kFbPlaybackPause, state ) );
}

void my_play_callback_static::on_playback_seek( double time )
{
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kFbPlaybackSeek, time ) );
}

void my_play_callback_static::on_playback_starting( play_control::t_track_command cmd, bool paused )
{
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kFbPlaybackStarting, static_cast<uint32_t>( cmd ), paused ) );
}

void my_play_callback_static::on_playback_stop( play_control::t_stop_reason reason )
{
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kFbPlaybackStop, static_cast<uint32_t>( reason ) ) );
}

void my_play_callback_static::on_playback_time( double time )
{
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kFbPlaybackTime, time ) );
}

void my_play_callback_static::on_volume_change( float newval )
{
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kFbVolumeChange, newval ) );
}

void my_playback_queue_callback::on_changed( t_change_origin p_origin )
{
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kFbPlaybackQueueChanged, static_cast<uint32_t>( p_origin ) ) );
}

void my_playback_statistics_collector::on_item_played( metadb_handle_ptr p_item )
{
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kFbItemPlayed, p_item ) );
}

my_config_object_notify::my_config_object_notify()
    : watchedObjects_{ { { standard_config_objects::bool_playlist_stop_after_current, EventId::kFbPlaylistStopAfterCurrentChanged },
                         { standard_config_objects::bool_cursor_follows_playback, EventId::kFbCursorFollowPlaybackChanged },
                         { standard_config_objects::bool_playback_follows_cursor, EventId::kFbPlaybackFollowCursorChanged },
                         { standard_config_objects::bool_ui_always_on_top, EventId::kFbAlwaysOnTopChanged } } }
{
}

GUID my_config_object_notify::get_watched_object( t_size p_index )
{
    if ( p_index >= watchedObjects_.size() )
    {
        return pfc::guid_null;
    }

    return watchedObjects_[p_index].first;
}

t_size my_config_object_notify::get_watched_object_count()
{
    return watchedObjects_.size();
}

void my_config_object_notify::on_watched_object_changed( const config_object::ptr& p_object )
{
    const auto it = ranges::find_if( watchedObjects_,
                                     [guid = p_object->get_guid()]( const auto& elem ) { return elem.first == guid; } );
    if ( watchedObjects_.cend() == it )
    {
        return;
    }

    bool boolval;
    p_object->get_data_bool( boolval );
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( it->second, boolval ) );
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
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kFbPlaylistItemEnsureVisible, p_playlist, p_idx ) );
}

void my_playlist_callback_static::on_item_focus_change( t_size p_playlist, t_size p_from, t_size p_to )
{
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kFbItemFocusChange, p_playlist, p_from, p_to ) );
}

void my_playlist_callback_static::on_items_added( t_size p_playlist, t_size, metadb_handle_list_cref, const pfc::bit_array& )
{
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kFbPlaylistItemsAdded, p_playlist ) );
}

void my_playlist_callback_static::on_items_removing( t_size p_playlist, const pfc::bit_array& p_mask, t_size p_old_count, t_size p_new_count )
{
}

void my_playlist_callback_static::on_items_removed( t_size p_playlist, const pfc::bit_array&, t_size, t_size p_new_count )
{
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kFbPlaylistItemsRemoved, p_playlist, p_new_count ) );
}

void my_playlist_callback_static::on_items_reordered( t_size p_playlist, const t_size*, t_size )
{
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kFbPlaylistItemsReordered, p_playlist ) );
}

void my_playlist_callback_static::on_items_replaced( t_size p_playlist, const pfc::bit_array& p_mask, const pfc::list_base_const_t<t_on_items_replaced_entry>& p_data )
{
}

void my_playlist_callback_static::on_items_selection_change( t_size, const pfc::bit_array&, const pfc::bit_array& )
{
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kFbPlaylistItemsSelectionChange ) );
}

void my_playlist_callback_static::on_items_modified( t_size p_playlist, const pfc::bit_array& p_mask )
{
}

void my_playlist_callback_static::on_items_modified_fromplayback( t_size p_playlist, const pfc::bit_array& p_mask, play_control::t_display_level p_level )
{
}

void my_playlist_callback_static::on_playback_order_changed( t_size p_new_index )
{
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kFbPlaybackOrderChanged, p_new_index ) );
}

void my_playlist_callback_static::on_playlist_activate( t_size p_old, t_size p_new )
{
    if ( p_old != p_new )
    {
        on_playlist_switch();
    }
}

void my_playlist_callback_static::on_playlist_created( t_size, const char*, t_size )
{
    on_playlists_changed();
}

void my_playlist_callback_static::on_playlist_locked( t_size, bool )
{
    on_playlists_changed();
}

void my_playlist_callback_static::on_playlist_renamed( t_size, const char*, t_size )
{
    on_playlists_changed();
}

void my_playlist_callback_static::on_playlists_removing( const pfc::bit_array&, t_size, t_size )
{
}

void my_playlist_callback_static::on_playlists_removed( const pfc::bit_array&, t_size, t_size )
{
    on_playlists_changed();
}

void my_playlist_callback_static::on_playlists_reorder( const t_size*, t_size )
{
    on_playlists_changed();
}

void my_playlist_callback_static::on_playlist_switch()
{
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kFbPlaylistSwitch ) );
}

void my_playlist_callback_static::on_playlists_changed()
{
    EventDispatcher::Get().PutEventToAll( GenerateEvent_JsCallback( EventId::kFbPlaylistsChanged ) );
}

} // namespace

namespace
{

FB2K_SERVICE_FACTORY( InitStageCallback );
FB2K_SERVICE_FACTORY( my_initquit );
FB2K_SERVICE_FACTORY( my_library_callback );
FB2K_SERVICE_FACTORY( my_play_callback_static );
FB2K_SERVICE_FACTORY( my_playback_queue_callback );
FB2K_SERVICE_FACTORY( my_playback_statistics_collector );
FB2K_SERVICE_FACTORY( my_config_object_notify );
FB2K_SERVICE_FACTORY( my_dsp_config_callback );
FB2K_SERVICE_FACTORY( my_metadb_io_callback );
FB2K_SERVICE_FACTORY( my_playlist_callback_static );

} // namespace
