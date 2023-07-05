#include <stdafx.h>

#include "playback_control.h"

#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/dom/event.h>
#include <js_backend/objects/fb2k/events/playback_stop_event.h>
#include <js_backend/objects/fb2k/track.h>
#include <tasks/events/playback_stop_event.h>
#include <utils/float_comparators.h>

using namespace smp;

namespace
{

const std::unordered_map<qwr::u8string, uint32_t> kPlaybackOrderStrToIdx{
    { "normal", 0 },
    { "repeat-playlist", 1 },
    { "repeat-track", 2 },
    { "random", 3 },
    { "shuffle-tracks", 4 },
    { "shuffle-albums", 5 },
    { "shuffle-folder", 6 },
};
const auto kPlaybackOrderIdxToStr = kPlaybackOrderStrToIdx
                                    | ranges::views::transform( []( const auto& elem ) { return std::make_pair( elem.second, elem.first ); } )
                                    | ranges::to<std::unordered_map>();

} // namespace

namespace
{

auto GenerateStopEventProps( const smp::PlaybackStopEvent& event )
{
    mozjs::PlaybackStopEvent::EventProperties props{
        .baseProps = mozjs::JsEvent::EventProperties{ .cancelable = false },
        .reason = [&] {
            switch ( event.GetReason() )
            {
            case play_control::stop_reason_user:
                return "user";
            case play_control::stop_reason_eof:
                return "eof";
            case play_control::stop_reason_starting_another:
                return "starting-another";
            case play_control::stop_reason_shutting_down:
                return "shutting-down";
            default:
                assert( false );
                return "unknown";
            }
        }()
    };

    return props;
}

float DbToFraction( float db )
{
    if ( IsEpsilonEqual( db, -100 ) )
    {
        return 0;
    }

    return std::clamp( std::powf( 2.0f, db / 10.0f ), 0.0f, 1.0f );
}

float FractionToDb( float fraction )
{
    if ( IsEpsilonEqual( fraction, 0 ) )
    {
        return -100;
    }

    return std::clamp( 10 * std::log2f( fraction ), -100.0f, 0.0f );
}

} // namespace

namespace
{

using namespace mozjs;

JSClassOps jsOps = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    JsObjectBase<PlaybackControl>::FinalizeJsObject,
    nullptr,
    nullptr,
    PlaybackControl::Trace
};

JSClass jsClass = {
    "PlaybackControl",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( getCurrentlyPlayingTrack, PlaybackControl::GetCurrentlyPlayingTrack )
MJS_DEFINE_JS_FN_FROM_NATIVE( next, PlaybackControl::Next )
MJS_DEFINE_JS_FN_FROM_NATIVE( pause, PlaybackControl::Pause )
MJS_DEFINE_JS_FN_FROM_NATIVE( play, PlaybackControl::Play )
MJS_DEFINE_JS_FN_FROM_NATIVE( playRandom, PlaybackControl::PlayRandom )
MJS_DEFINE_JS_FN_FROM_NATIVE( previous, PlaybackControl::Previous )
MJS_DEFINE_JS_FN_FROM_NATIVE( stop, PlaybackControl::Stop )
MJS_DEFINE_JS_FN_FROM_NATIVE( volumeDown, PlaybackControl::VolumeDown )
MJS_DEFINE_JS_FN_FROM_NATIVE( volumeMute, PlaybackControl::VolumeMute )
MJS_DEFINE_JS_FN_FROM_NATIVE( volumeUp, PlaybackControl::VolumeUp )

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "getCurrentlyPlayingTrack", getCurrentlyPlayingTrack, 0, kDefaultPropsFlags ),
        JS_FN( "next", next, 0, kDefaultPropsFlags ),
        JS_FN( "pause", pause, 0, kDefaultPropsFlags ),
        JS_FN( "play", play, 0, kDefaultPropsFlags ),
        JS_FN( "playRandom", playRandom, 0, kDefaultPropsFlags ),
        JS_FN( "previous", previous, 0, kDefaultPropsFlags ),
        JS_FN( "stop", stop, 0, kDefaultPropsFlags ),
        JS_FN( "volumeDown", volumeDown, 0, kDefaultPropsFlags ),
        JS_FN( "volumeMute", volumeMute, 0, kDefaultPropsFlags ),
        JS_FN( "volumeUp", volumeUp, 0, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_currentTime, PlaybackControl::get_CurrentTime )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_cursorFollowsPlayback, PlaybackControl::get_CursorFollowsPlayback )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_paused, PlaybackControl::get_IsPaused )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_playbackFollowsCursor, PlaybackControl::get_PlaybackFollowsCursor )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_playing, PlaybackControl::get_IsPlaying )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_stopAfterCurrent, PlaybackControl::get_StopAfterCurrentTrack )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_volume, PlaybackControl::get_Volume )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_currentTime, PlaybackControl::put_CurrentTime )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_cursorFollowsPlayback, PlaybackControl::put_CursorFollowsPlayback )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_playbackFollowsCursor, PlaybackControl::put_PlaybackFollowsCursor )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_stopAfterCurrent, PlaybackControl::put_StopAfterCurrentTrack )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_volume, PlaybackControl::put_Volume )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSGS( "currentTime", get_currentTime, put_currentTime, kDefaultPropsFlags ),
        JS_PSGS( "cursorFollowsPlayback", get_cursorFollowsPlayback, put_cursorFollowsPlayback, kDefaultPropsFlags ),
        JS_PSG( "paused", get_paused, kDefaultPropsFlags ),
        JS_PSGS( "playbackFollowsCursor", get_playbackFollowsCursor, put_playbackFollowsCursor, kDefaultPropsFlags ),
        JS_PSG( "playing", get_playing, kDefaultPropsFlags ),
        JS_PSGS( "stopAfterCurrentTrack", get_stopAfterCurrent, put_stopAfterCurrent, kDefaultPropsFlags ),
        JS_PSGS( "volume", get_volume, put_volume, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_VERIFY_OBJECT( mozjs::PlaybackControl );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<PlaybackControl>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<PlaybackControl>::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsObjectTraits<PlaybackControl>::JsProperties = jsProperties.data();
const PostJsCreateFn JsObjectTraits<PlaybackControl>::PostCreate = PlaybackControl::PostCreate;

const std::unordered_set<EventId> PlaybackControl::kHandledEvents{
    EventId::kNew_FbCfgCursorFollowsPlaybackChanged,
    EventId::kNew_FbCfgPlaybackFollowsCursorChanged,
    EventId::kNew_FbCfgPlaylistStopAfterCurrentChanged,
    EventId::kNew_FbPlaybackDynamicInfo,
    EventId::kNew_FbPlaybackDynamicInfoTrack,
    EventId::kNew_FbPlaybackEdited,
    EventId::kNew_FbPlaybackNewTrack,
    EventId::kNew_FbPlaybackPause,
    EventId::kNew_FbPlaybackPlay,
    EventId::kNew_FbPlaybackSeek,
    EventId::kNew_FbPlaybackStarting,
    EventId::kNew_FbPlaybackStop,
    EventId::kNew_FbPlaybackTime,
    EventId::kNew_FbPlaybackVolumeChange,
    EventId::kNew_FbPlaylistPlaybackOrderChanged,
};

PlaybackControl::PlaybackControl( JSContext* cx )
    : JsEventTarget( cx )
    , pJsCtx_( cx )
{
}

std::unique_ptr<PlaybackControl>
PlaybackControl::CreateNative( JSContext* cx )
{
    return std::unique_ptr<PlaybackControl>( new PlaybackControl( cx ) );
}

size_t PlaybackControl::GetInternalSize() const
{
    return 0;
}

void PlaybackControl::PostCreate( JSContext* cx, JS::HandleObject self )
{
    utils::CreateAndInstallPrototype<JsObjectBase<mozjs::PlaybackStopEvent>>( cx, self );
}

void PlaybackControl::Trace( JSTracer* trc, JSObject* obj )
{
    JsEventTarget::Trace( trc, obj );
}

const std::string& PlaybackControl::EventIdToType( smp::EventId eventId )
{
    static const std::unordered_map<EventId, std::string> idToType{
        { EventId::kNew_FbCfgCursorFollowsPlaybackChanged, "cursorFollowsPlaybackChange" },
        { EventId::kNew_FbCfgPlaybackFollowsCursorChanged, "playbackFollowsCursorChange" },
        { EventId::kNew_FbCfgPlaylistStopAfterCurrentChanged, "stopAfterCurrentTrackChange" },
        { EventId::kNew_FbPlaybackDynamicInfo, "trackDynamicInfoChange" },
        { EventId::kNew_FbPlaybackDynamicInfoTrack, "trackStreamInfoChange" },
        { EventId::kNew_FbPlaybackEdited, "trackInfoChange" },
        { EventId::kNew_FbPlaybackNewTrack, "start" },
        { EventId::kNew_FbPlaybackPause, "pause" },
        { EventId::kNew_FbPlaybackPlay, "play" },
        { EventId::kNew_FbPlaybackSeek, "seek" },
        { EventId::kNew_FbPlaybackStarting, "starting" },
        { EventId::kNew_FbPlaybackStop, "stop" },
        { EventId::kNew_FbPlaybackTime, "timeUpdate" },
        { EventId::kNew_FbPlaybackVolumeChange, "volumeChange" },
        { EventId::kNew_FbPlaylistPlaybackOrderChanged, "playbackOrderChange" },
    };

    assert( idToType.size() == kHandledEvents.size() );
    assert( idToType.contains( eventId ) );
    return idToType.at( eventId );
}

EventStatus PlaybackControl::HandleEvent( JS::HandleObject self, const smp::EventBase& event )
{
    EventStatus status;

    const auto& eventType = EventIdToType( event.GetId() );
    if ( !HasEventListener( eventType ) )
    {
        return status;
    }
    status.isHandled = true;

    JS::RootedValue jsEvent( pJsCtx_ );
    if ( event.GetId() == EventId::kNew_FbPlaybackStop )
    {
        const auto& playbackStopEvent = static_cast<const smp::PlaybackStopEvent&>( event );
        jsEvent.setObjectOrNull(
            mozjs::JsObjectBase<PlaybackStopEvent>::CreateJs(
                pJsCtx_,
                eventType,
                GenerateStopEventProps( playbackStopEvent ) ) );
    }
    else
    {
        jsEvent.setObjectOrNull( mozjs::JsEvent::CreateJs( pJsCtx_, eventType, JsEvent::EventProperties{ .cancelable = false } ) );
    }
    DispatchEvent( self, jsEvent );

    const auto pNativeEvent = convert::to_native::ToValue<JsEvent*>( pJsCtx_, jsEvent );
    status.isDefaultSuppressed = pNativeEvent->get_DefaultPrevented();
    return status;
}

JSObject* PlaybackControl::GetCurrentlyPlayingTrack()
{
    metadb_handle_ptr metadbHandle;
    if ( !playback_control::get()->get_now_playing( metadbHandle ) )
    {
        return nullptr;
    }

    return Track::CreateJs( pJsCtx_, metadbHandle );
}

void PlaybackControl::Next()
{
    standard_commands::main_next();
}

void PlaybackControl::Pause()
{
    standard_commands::main_pause();
}

void PlaybackControl::Play()
{
    standard_commands::main_play();
}

void PlaybackControl::PlayRandom()
{
    standard_commands::main_random();
}

void PlaybackControl::Previous()
{
    standard_commands::main_previous();
}

void PlaybackControl::Stop()
{
    standard_commands::main_stop();
}

void PlaybackControl::VolumeDown()
{
    standard_commands::main_volume_down();
}

void PlaybackControl::VolumeMute()
{
    standard_commands::main_volume_mute();
}

void PlaybackControl::VolumeUp()
{
    standard_commands::main_volume_up();
}

double PlaybackControl::get_CurrentTime() const
{
    return playback_control::get()->playback_get_position();
}

bool PlaybackControl::get_CursorFollowsPlayback() const
{
    return config_object::g_get_data_bool_simple( standard_config_objects::bool_cursor_follows_playback, false );
}

bool PlaybackControl::get_IsPaused() const
{
    return playback_control::get()->is_paused();
}

bool PlaybackControl::get_IsPlaying() const
{
    return playback_control::get()->is_playing();
}

bool PlaybackControl::get_PlaybackFollowsCursor() const
{
    return config_object::g_get_data_bool_simple( standard_config_objects::bool_playback_follows_cursor, false );
}

qwr::u8string PlaybackControl::get_PlaybackOrder() const
{
    const auto idx = playlist_manager::get()->playback_order_get_active();
    assert( kPlaybackOrderIdxToStr.contains( idx ) );
    return kPlaybackOrderIdxToStr.at( idx );
}

bool PlaybackControl::get_StopAfterCurrentTrack() const
{
    return playback_control::get()->get_stop_after_current();
}

float PlaybackControl::get_Volume() const
{
    return DbToFraction( playback_control::get()->get_volume() );
}

void PlaybackControl::put_CurrentTime( double time )
{
    playback_control::get()->playback_seek( time );
}

void PlaybackControl::put_CursorFollowsPlayback( bool value )
{
    config_object::g_set_data_bool( standard_config_objects::bool_cursor_follows_playback, value );
}

void PlaybackControl::put_PlaybackFollowsCursor( bool value )
{
    config_object::g_set_data_bool( standard_config_objects::bool_playback_follows_cursor, value );
}

void PlaybackControl::put_PlaybackOrder( const qwr::u8string& value )
{
    qwr::QwrException::ExpectTrue( kPlaybackOrderStrToIdx.contains( value ), "Unknown playback order value" );
    playlist_manager::get()->playback_order_set_active( kPlaybackOrderStrToIdx.at( value ) );
}

void PlaybackControl::put_StopAfterCurrentTrack( bool value )
{
    playback_control::get()->set_stop_after_current( value );
}

void PlaybackControl::put_Volume( float value )
{
    playback_control::get()->set_volume( FractionToDb( std::clamp( value, 0.0f, 1.0f ) ) );
}

} // namespace mozjs
