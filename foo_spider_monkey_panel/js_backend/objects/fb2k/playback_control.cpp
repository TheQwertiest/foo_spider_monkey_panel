#include <stdafx.h>

#include "playback_control.h"

#include <events/playback_stop_event.h>
#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/dom/event.h>
#include <js_backend/objects/fb2k/fb_metadb_handle.h>
#include <js_backend/objects/fb2k/playback_stop_event.h>

using namespace smp;

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
    nullptr,
    PlaybackControl::Trace
};

JSClass jsClass = {
    "PlaybackControl",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( getNowPlayingTrack, PlaybackControl::GetNowPlayingTrack )
MJS_DEFINE_JS_FN_FROM_NATIVE( next, PlaybackControl::Next )
MJS_DEFINE_JS_FN_FROM_NATIVE( pause, PlaybackControl::Pause )
MJS_DEFINE_JS_FN_FROM_NATIVE( play, PlaybackControl::Play )
MJS_DEFINE_JS_FN_FROM_NATIVE( prev, PlaybackControl::Prev )
MJS_DEFINE_JS_FN_FROM_NATIVE( random, PlaybackControl::Random )
MJS_DEFINE_JS_FN_FROM_NATIVE( stop, PlaybackControl::Stop )
MJS_DEFINE_JS_FN_FROM_NATIVE( volumeDown, PlaybackControl::VolumeDown )
MJS_DEFINE_JS_FN_FROM_NATIVE( volumeMute, PlaybackControl::VolumeMute )
MJS_DEFINE_JS_FN_FROM_NATIVE( volumeUp, PlaybackControl::VolumeUp )

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "getNowPlayingTrack", getNowPlayingTrack, 0, kDefaultPropsFlags ),
        JS_FN( "next", next, 0, kDefaultPropsFlags ),
        JS_FN( "pause", pause, 0, kDefaultPropsFlags ),
        JS_FN( "play", play, 0, kDefaultPropsFlags ),
        JS_FN( "prev", prev, 0, kDefaultPropsFlags ),
        JS_FN( "random", random, 0, kDefaultPropsFlags ),
        JS_FN( "stop", stop, 0, kDefaultPropsFlags ),
        JS_FN( "volumeDown", volumeDown, 0, kDefaultPropsFlags ),
        JS_FN( "volumeMute", volumeMute, 0, kDefaultPropsFlags ),
        JS_FN( "volumeUp", volumeUp, 0, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_paused, PlaybackControl::get_IsPaused )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_playing, PlaybackControl::get_IsPlaying )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_currentTime, PlaybackControl::get_CurrentTime )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_stopAfterCurrent, PlaybackControl::get_StopAfterCurrent )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_volume, PlaybackControl::get_Volume )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_playbackTime, PlaybackControl::put_CurrentTime )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_stopAfterCurrent, PlaybackControl::put_StopAfterCurrent )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_volume, PlaybackControl::put_Volume )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "paused", get_paused, kDefaultPropsFlags ),
        JS_PSG( "playing", get_playing, kDefaultPropsFlags ),
        JS_PSGS( "currentTime", get_currentTime, put_playbackTime, kDefaultPropsFlags ),
        JS_PSGS( "stopAfterCurrent", get_stopAfterCurrent, put_stopAfterCurrent, kDefaultPropsFlags ),
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
    EventId::kNew_FbPlaybackDynamicInfo,
    EventId::kNew_FbPlaybackDynamicInfoTrack,
    EventId::kNew_FbPlaybackEdited,
    EventId::kNew_FbPlaybackNewTrack,
    EventId::kNew_FbPlaybackPlay,
    EventId::kNew_FbPlaybackPause,
    EventId::kNew_FbPlaybackSeek,
    EventId::kNew_FbPlaybackStarting,
    EventId::kNew_FbPlaybackStop,
    EventId::kNew_FbPlaybackTime,
    EventId::kNew_FbPlaybackVolumeChange
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

size_t PlaybackControl::GetInternalSize()
{
    return 0;
}

void PlaybackControl::PostCreate( JSContext* cx, JS::HandleObject self )
{
    utils::CreateAndInstallPrototype<JsObjectBase<mozjs::PlaybackStopEvent>>( cx, self, JsPrototypeId::New_PlaybackStopEvent );

    {
        JS::RootedObject jsObject( cx, JS_NewPlainObject( cx ) );
        JsException::ExpectTrue( jsObject );

        static const auto props = std::to_array<JSPropertySpec>(
            {
                JS_INT32_PS( "STOP_REASON_USER", playback_control::stop_reason_user, kDefaultPropsFlags | JSPROP_READONLY ),
                JS_INT32_PS( "STOP_REASON_EOF", playback_control::stop_reason_eof, kDefaultPropsFlags | JSPROP_READONLY ),
                JS_INT32_PS( "STOP_REASON_STARTING_ANOTHER", playback_control::stop_reason_starting_another, kDefaultPropsFlags | JSPROP_READONLY ),
                JS_INT32_PS( "STOP_REASON_SHUTTING_DOWN", playback_control::stop_reason_shutting_down, kDefaultPropsFlags | JSPROP_READONLY ),
                JS_PS_END,
            } );
        if ( !JS_DefineProperties( cx, jsObject, props.data() ) )
        {
            throw smp::JsException();
        }

        if ( !JS_DefineProperty( cx, self, "constants", jsObject, JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY ) )
        {
            throw JsException();
        }
    }
}

void PlaybackControl::Trace( JSTracer* trc, JSObject* obj )
{
    JsEventTarget::Trace( trc, obj );
}

void PlaybackControl::PrepareForGc()
{
    JsEventTarget::PrepareForGc();
}

const std::string& PlaybackControl::EventIdToType( smp::EventId eventId )
{
    static const std::unordered_map<EventId, std::string> idToType{
        { EventId::kNew_FbPlaybackDynamicInfo, "trackDynamicInfoChange" },
        { EventId::kNew_FbPlaybackDynamicInfoTrack, "trackStreamInfoChange" },
        { EventId::kNew_FbPlaybackEdited, "trackInfoEdit" },
        { EventId::kNew_FbPlaybackNewTrack, "start" },
        { EventId::kNew_FbPlaybackPlay, "play" },
        { EventId::kNew_FbPlaybackPause, "pause" },
        { EventId::kNew_FbPlaybackSeek, "seek" },
        { EventId::kNew_FbPlaybackStarting, "starting" },
        { EventId::kNew_FbPlaybackStop, "stop" },
        { EventId::kNew_FbPlaybackTime, "timeUpdate" },
        { EventId::kNew_FbPlaybackVolumeChange, "volumeChange" }
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
        jsEvent.setObjectOrNull(
            mozjs::JsObjectBase<PlaybackStopEvent>::CreateJs(
                pJsCtx_,
                eventType,
                static_cast<const smp::PlaybackStopEvent&>( event ).GetReason() ) );
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

JSObject* PlaybackControl::GetNowPlayingTrack()
{
    metadb_handle_ptr metadb;
    if ( !playback_control::get()->get_now_playing( metadb ) )
    {
        return nullptr;
    }

    return JsFbMetadbHandle::CreateJs( pJsCtx_, metadb );
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

void PlaybackControl::Prev()
{
    standard_commands::main_previous();
}

void PlaybackControl::Random()
{
    standard_commands::main_random();
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

bool PlaybackControl::get_IsPaused()
{
    return playback_control::get()->is_paused();
}

bool PlaybackControl::get_IsPlaying()
{
    return playback_control::get()->is_playing();
}

double PlaybackControl::get_CurrentTime()
{
    return playback_control::get()->playback_get_position();
}

bool PlaybackControl::get_StopAfterCurrent()
{
    return playback_control::get()->get_stop_after_current();
}

float PlaybackControl::get_Volume()
{
    return playback_control::get()->get_volume();
}

void PlaybackControl::put_CurrentTime( double time )
{
    playback_control::get()->playback_seek( time );
}

void PlaybackControl::put_StopAfterCurrent( bool p )
{
    playback_control::get()->set_stop_after_current( p );
}

void PlaybackControl::put_Volume( float value )
{
    playback_control::get()->set_volume( value );
}

} // namespace mozjs
