#include <stdafx.h>

#include <events/dispatcher/event_dispatcher.h>
#include <events/playback_stop_event.h>

using namespace smp;

namespace
{

class PlayCallbackImpl : public play_callback_static
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

} // namespace

namespace
{

unsigned PlayCallbackImpl::get_flags()
{
    return flag_on_playback_all | flag_on_volume_change;
}

void PlayCallbackImpl::on_playback_dynamic_info( const file_info& /*info*/ )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<JsCallbackEventNew>( EventId::kNew_FbPlaybackDynamicInfo ) );
}

void PlayCallbackImpl::on_playback_dynamic_info_track( const file_info& /*info*/ )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<JsCallbackEventNew>( EventId::kNew_FbPlaybackDynamicInfoTrack ) );
}

void PlayCallbackImpl::on_playback_edited( metadb_handle_ptr /*track*/ )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<JsCallbackEventNew>( EventId::kNew_FbPlaybackEdited ) );
}

void PlayCallbackImpl::on_playback_new_track( metadb_handle_ptr /*track*/ )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<JsCallbackEventNew>( EventId::kNew_FbPlaybackNewTrack ) );
}

void PlayCallbackImpl::on_playback_pause( bool state )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<JsCallbackEventNew>( state ? EventId::kNew_FbPlaybackPause : EventId::kNew_FbPlaybackPlay ) );
}

void PlayCallbackImpl::on_playback_seek( double /*time*/ )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<JsCallbackEventNew>( EventId::kNew_FbPlaybackSeek ) );
}

void PlayCallbackImpl::on_playback_starting( play_control::t_track_command /*cmd*/, bool /*paused*/ )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<JsCallbackEventNew>( EventId::kNew_FbPlaybackStarting ) );
}

void PlayCallbackImpl::on_playback_stop( play_control::t_stop_reason reason )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<PlaybackStopEvent>( EventId::kNew_FbPlaybackStop, reason ) );
}

void PlayCallbackImpl::on_playback_time( double /*time*/ )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<JsCallbackEventNew>( EventId::kNew_FbPlaybackTime ) );
}

void PlayCallbackImpl::on_volume_change( float /*newval*/ )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<JsCallbackEventNew>( EventId::kNew_FbPlaybackVolumeChange ) );
}

} // namespace

namespace
{

FB2K_SERVICE_FACTORY( PlayCallbackImpl );

} // namespace
