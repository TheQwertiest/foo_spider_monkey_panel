#include <stdafx.h>

#include "playback_queue.h"

#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/dom/event.h>
#include <js_backend/objects/fb2k/events/playback_queue_event.h>
#include <js_backend/objects/fb2k/track.h>
#include <js_backend/utils/js_property_helper.h>
#include <tasks/events/playback_queue_event.h>

using namespace smp;

namespace
{

auto GenerateQueueEventProps( const smp::PlaybackQueueEvent& event )
{
    mozjs::PlaybackQueueEvent::EventProperties props{
        .baseProps = mozjs::JsEvent::EventProperties{ .cancelable = false },
        .origin = [&] {
            switch ( event.GetOrigin() )
            {
            case playback_queue_callback::changed_user_added:
                return "add-by-user";
            case playback_queue_callback::changed_user_removed:
                return "remove-by-user";
            case playback_queue_callback::changed_playback_advance:
                return "playback-advance";
            default:
                assert( false );
                return "unknown";
            }
        }()
    };

    return props;
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
    JsObjectBase<PlaybackQueue>::FinalizeJsObject,
    nullptr,
    nullptr,
    PlaybackQueue::Trace
};

JSClass jsClass = {
    "PlaybackQueue",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( clear, PlaybackQueue::Clear )
MJS_DEFINE_JS_FN_FROM_NATIVE( toArray, PlaybackQueue::ToArray )
MJS_DEFINE_JS_FN_FROM_NATIVE( indexOf, PlaybackQueue::IndexOf )
MJS_DEFINE_JS_FN_FROM_NATIVE( pullAt, PlaybackQueue::PullAt )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( push, PlaybackQueue::Push_Fake, PlaybackQueue::PushWithOpt, 1 )

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "clear", clear, 0, kDefaultPropsFlags ),
        JS_FN( "indexOf", indexOf, 3, kDefaultPropsFlags ),
        JS_FN( "pullAt", pullAt, 1, kDefaultPropsFlags ),
        JS_FN( "push", push, 1, kDefaultPropsFlags ),
        JS_FN( "toArray", toArray, 0, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_length, PlaybackQueue::get_Length );

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "length", get_length, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_VERIFY_OBJECT( mozjs::PlaybackQueue );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<PlaybackQueue>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<PlaybackQueue>::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsObjectTraits<PlaybackQueue>::JsProperties = jsProperties.data();
const PostJsCreateFn JsObjectTraits<PlaybackQueue>::PostCreate = PlaybackQueue::PostCreate;

const std::unordered_set<smp::EventId> PlaybackQueue::kHandledEvents{
    EventId::kNew_FbPlaybackQueueChanged,
};

PlaybackQueue::PlaybackQueue( JSContext* cx )
    : JsEventTarget( cx )
    , pJsCtx_( cx )
{
}

PlaybackQueue::~PlaybackQueue()
{
}

std::unique_ptr<PlaybackQueue>
PlaybackQueue::CreateNative( JSContext* cx )
{
    return std::unique_ptr<PlaybackQueue>( new PlaybackQueue( cx ) );
}

size_t PlaybackQueue::GetInternalSize() const
{
    return 0;
}

void PlaybackQueue::PostCreate( JSContext* cx, JS::HandleObject self )
{
    utils::CreateAndInstallPrototype<JsObjectBase<mozjs::PlaybackQueueEvent>>( cx, self );
}

void PlaybackQueue::Trace( JSTracer* trc, JSObject* obj )
{
    JsEventTarget::Trace( trc, obj );
}

const std::string& PlaybackQueue::EventIdToType( smp::EventId eventId ) const
{
    static const std::unordered_map<EventId, std::string> idToType{
        { EventId::kNew_FbPlaybackQueueChanged, "change" },
    };

    assert( idToType.contains( eventId ) );
    return idToType.at( eventId );
}

EventStatus PlaybackQueue::HandleEvent( JS::HandleObject self, const smp::EventBase& event )
{
    EventStatus status;

    const auto& eventType = EventIdToType( event.GetId() );
    if ( !HasEventListener( eventType ) )
    {
        return status;
    }
    status.isHandled = true;

    JS::RootedValue jsEvent( pJsCtx_ );
    if ( event.GetId() == EventId::kNew_FbPlaybackQueueChanged )
    {
        const auto& playbackQueueEvent = static_cast<const smp::PlaybackQueueEvent&>( event );
        jsEvent.setObjectOrNull(
            mozjs::JsObjectBase<PlaybackQueueEvent>::CreateJs(
                pJsCtx_,
                eventType,
                GenerateQueueEventProps( playbackQueueEvent ) ) );
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

void PlaybackQueue::Clear()
{
    return playlist_manager_v6::get()->queue_flush();
}

int32_t PlaybackQueue::IndexOf( smp::not_null<const Track*> track, int32_t playlistIndex, int32_t trackIndex ) const
{
    return playlist_manager_v6::get()->queue_find_index( { .m_handle = track->GetHandle(),
                                                           .m_playlist = static_cast<uint32_t>( playlistIndex ),
                                                           .m_item = static_cast<uint32_t>( trackIndex ) } );
}

void PlaybackQueue::PullAt( const std::vector<uint32_t>& itemIndices )
{
    pfc::bit_array_bittable mask;
    for ( auto i: itemIndices )
    {
        mask.set( i, true );
    }
    playlist_manager_v6::get()->queue_remove_mask( mask );
}

void PlaybackQueue::Push_Fake( JS::HandleValue /*arg1*/, JS::HandleValue /*arg2*/ )
{
}

void PlaybackQueue::Push_1( smp::not_null<const Track*> track )
{
    playlist_manager_v6::get()->queue_add_item( track->GetHandle() );
}

void PlaybackQueue::Push_2( uint32_t playlistIndex, uint32_t trackIndex )
{
    playlist_manager_v6::get()->queue_add_item_playlist( playlistIndex, trackIndex );
}

void PlaybackQueue::PushWithOpt( size_t optArgCount, JS::HandleValue arg1, JS::HandleValue arg2 )
{
    switch ( optArgCount )
    {
    case 0:
    {
        const auto playlistIndex = convert::to_native::ToValue<uint32_t>( pJsCtx_, arg1 );
        const auto trackIndex = convert::to_native::ToValue<uint32_t>( pJsCtx_, arg2 );
        return Push_2( playlistIndex, trackIndex );
    }
    case 1:
    {
        auto pTrack = convert::to_native::ToValue<smp::not_null<Track*>>( pJsCtx_, arg1 );
        return Push_1( pTrack );
    }
    default:
        throw qwr::QwrException( "{} is not a valid argument count for any overload", 2 - optArgCount );
    }
}

JS::Value PlaybackQueue::ToArray() const
{
    pfc::list_t<t_playback_queue_item> items;
    playlist_manager::get()->queue_get_contents( items );

    JS::RootedValue jsValue( pJsCtx_ );
    // TODO: move to a custom object instead
    JS::RootedObject jsItem( pJsCtx_ );
    JS::RootedObject jsTrack( pJsCtx_ );

    convert::to_js::ToArrayValue(
        pJsCtx_,
        items.size(),
        [&]( auto i ) {
            jsItem = JS_NewPlainObject( pJsCtx_ );
            JsException::ExpectTrue( jsItem );

            const auto& item = items[i];
            jsTrack = Track::CreateJs( pJsCtx_, item.m_handle );
            utils::SetProperty( pJsCtx_, jsItem, "track", static_cast<JS::HandleObject>( jsTrack ) );
            utils::SetProperty( pJsCtx_, jsItem, "playlistIndex", static_cast<int32_t>( item.m_playlist ) );
            utils::SetProperty( pJsCtx_, jsItem, "trackIndex", static_cast<int32_t>( item.m_item ) );
            return jsItem.get();
        },
        &jsValue );
    return jsValue;
}

uint32_t PlaybackQueue::get_Length() const
{
    return playlist_manager_v6::get()->queue_get_count();
}

} // namespace mozjs
