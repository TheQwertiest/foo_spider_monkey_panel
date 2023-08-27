#include <stdafx.h>

#include "playlist_manager.h"

#include <fb2k/playlist_index_manager.h>
#include <fb2k/playlist_lock.h>
#include <fb2k/title_format_manager.h>
#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/dom/event.h>
#include <js_backend/objects/fb2k/events/multi_playlist_event.h>
#include <js_backend/objects/fb2k/events/playlist_event.h>
#include <js_backend/objects/fb2k/events/playlist_multi_track_event.h>
#include <js_backend/objects/fb2k/events/playlist_track_event.h>
#include <js_backend/objects/fb2k/playlist.h>
#include <js_backend/objects/fb2k/playlist_recycle_bin.h>
#include <js_backend/objects/fb2k/track.h>
#include <js_backend/objects/fb2k/track_list.h>
#include <js_backend/utils/js_property_helper.h>
#include <tasks/events/multi_playlist_event.h>
#include <tasks/events/playlist_event.h>
#include <tasks/events/playlist_item_event.h>
#include <tasks/events/playlist_multi_item_event.h>
#include <utils/text_helpers.h>

using namespace smp;

namespace
{

auto GeneratePlaylistEventProps( const smp::PlaylistEvent& event )
{
    mozjs::PlaylistEvent::EventProperties props{
        .baseProps = mozjs::JsEvent::EventProperties{ .cancelable = false },
        .playlistIndex = event.GetPlaylistIndex()
    };

    return props;
}

auto GenerateMultiPlaylistEventProps( const smp::MultiPlaylistEvent& event )
{
    mozjs::MultiPlaylistEvent::EventProperties props{
        .baseProps = mozjs::JsEvent::EventProperties{ .cancelable = false },
        .playlistIndices = event.GetPlaylistIndices()
    };

    return props;
}

auto GenerateTrackEventProps( const smp::PlaylistItemEvent& event )
{
    mozjs::PlaylistTrackEvent::EventProperties props{
        .baseProps = GeneratePlaylistEventProps( event ),
        .trackIndex = event.GetItemIndex()
    };

    return props;
}

auto GenerateMultiTrackEventProps( const smp::PlaylistMultiItemEvent& event )
{
    mozjs::PlaylistMultiTrackEvent::EventProperties props{
        .baseProps = GeneratePlaylistEventProps( event ),
        .trackIndices = event.GetItemIndices()
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
    JsObjectBase<PlaylistManager>::FinalizeJsObject,
    nullptr,
    nullptr,
    PlaylistManager::Trace
};

JSClass jsClass = {
    "PlaylistManager",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( createAutoPlaylist, PlaylistManager::CreateAutoPlaylist, PlaylistManager::CreateAutoPlaylistWithOpt, 4 );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( createPlaylist, PlaylistManager::CreatePlaylist, PlaylistManager::CreatePlaylistWithOpt, 2 );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( deletePlaylist, PlaylistManager::DeletePlaylist, PlaylistManager::DeletePlaylistWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( duplicatePlaylist, PlaylistManager::DuplicatePlaylist, PlaylistManager::DuplicatePlaylistWithOpt, 2 );
MJS_DEFINE_JS_FN_FROM_NATIVE( findPlaylist, PlaylistManager::FindPlaylist );
MJS_DEFINE_JS_FN_FROM_NATIVE( getActivePlaylist, PlaylistManager::GetActivePlaylist );
MJS_DEFINE_JS_FN_FROM_NATIVE( getCurrentlyPlayingPlaylist, PlaylistManager::GetCurrentlyPlayingPlaylist );
MJS_DEFINE_JS_FN_FROM_NATIVE( getCurrentlyPlayingTrackLocation, PlaylistManager::GetCurrentlyPlayingTrackLocation );
MJS_DEFINE_JS_FN_FROM_NATIVE( getPlaylist, PlaylistManager::GetPlaylist );
MJS_DEFINE_JS_FN_FROM_NATIVE( getPlaylistCount, PlaylistManager::GetPlaylistCount );
MJS_DEFINE_JS_FN_FROM_NATIVE( movePlaylist, PlaylistManager::MovePlaylist );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( orderPlaylistsByName, PlaylistManager::OrderPlaylistsByName, PlaylistManager::OrderPlaylistsByNameWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE( setActivePlaylistAsUiEditContext, PlaylistManager::SetActivePlaylistAsUiEditContext );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "createAutoPlaylist", createAutoPlaylist, 1, kDefaultPropsFlags ),
        JS_FN( "createPlaylist", createPlaylist, 0, kDefaultPropsFlags ),
        JS_FN( "deletePlaylist", deletePlaylist, 1, kDefaultPropsFlags ),
        JS_FN( "duplicatePlaylist", duplicatePlaylist, 1, kDefaultPropsFlags ),
        JS_FN( "findPlaylist", findPlaylist, 1, kDefaultPropsFlags ),
        JS_FN( "getActivePlaylist", getActivePlaylist, 0, kDefaultPropsFlags ),
        JS_FN( "getCurrentlyPlayingPlaylist", getCurrentlyPlayingPlaylist, 0, kDefaultPropsFlags ),
        JS_FN( "getCurrentlyPlayingTrackLocation", getCurrentlyPlayingTrackLocation, 0, kDefaultPropsFlags ),
        JS_FN( "getPlaylist", getPlaylist, 1, kDefaultPropsFlags ),
        JS_FN( "getPlaylistCount", getPlaylistCount, 0, kDefaultPropsFlags ),
        JS_FN( "movePlaylist", movePlaylist, 2, kDefaultPropsFlags ),
        JS_FN( "orderPlaylistsByName", orderPlaylistsByName, 0, kDefaultPropsFlags ),
        JS_FN( "setActivePlaylistAsUiEditContext", setActivePlaylistAsUiEditContext, 0, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_recycleBin, PlaylistManager::get_RecycleBin )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "recycleBin", get_recycleBin, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_VERIFY_OBJECT( mozjs::PlaylistManager );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<PlaylistManager>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<PlaylistManager>::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsObjectTraits<PlaylistManager>::JsProperties = jsProperties.data();
const PostJsCreateFn JsObjectTraits<PlaylistManager>::PostCreate = PlaylistManager::PostCreate;

const std::unordered_set<smp::EventId> PlaylistManager::kHandledEvents{
    EventId::kNew_FbPlaylistActivate,
    EventId::kNew_FbPlaylistCreated,
    EventId::kNew_FbPlaylistItemEnsureVisible,
    EventId::kNew_FbPlaylistItemFocusChange,
    EventId::kNew_FbPlaylistItemsAdded,
    EventId::kNew_FbPlaylistItemsModified,
    EventId::kNew_FbPlaylistItemsRemoved,
    EventId::kNew_FbPlaylistItemsReordered,
    EventId::kNew_FbPlaylistItemsReplaced,
    EventId::kNew_FbPlaylistItemsSelectionChange,
    EventId::kNew_FbPlaylistLocked,
    EventId::kNew_FbPlaylistRenamed,
    EventId::kNew_FbPlaylistsRemoved,
    EventId::kNew_FbPlaylistsReorder,
};

PlaylistManager::PlaylistManager( JSContext* cx )
    : JsEventTarget( cx )
    , pJsCtx_( cx )
{
}

PlaylistManager::~PlaylistManager()
{
}

std::unique_ptr<PlaylistManager>
PlaylistManager::CreateNative( JSContext* cx )
{
    return std::unique_ptr<PlaylistManager>( new PlaylistManager( cx ) );
}

size_t PlaylistManager::GetInternalSize() const
{
    return 0;
}

void PlaylistManager::PostCreate( JSContext* cx, JS::HandleObject self )
{
    utils::CreateAndInstallPrototype<JsObjectBase<mozjs::PlaylistEvent>>( cx, self );
    utils::CreateAndInstallPrototype<JsObjectBase<mozjs::PlaylistTrackEvent>>( cx, self );
    utils::CreateAndInstallPrototype<JsObjectBase<mozjs::PlaylistMultiTrackEvent>>( cx, self );
    utils::CreateAndInstallPrototype<JsObjectBase<mozjs::MultiPlaylistEvent>>( cx, self );
}

void PlaylistManager::Trace( JSTracer* trc, JSObject* obj )
{
    JsEventTarget::Trace( trc, obj );

    auto pNative = JsObjectBase<PlaylistManager>::ExtractNativeUnchecked( obj );
    if ( !pNative )
    {
        return;
    }

    for ( auto& value: ranges::views::values( pNative->idToPlaylist_ ) )
    {
        JS::TraceEdge( trc, &value, "Heap: PlaylistManager: playlist" );
    }
    JS::TraceEdge( trc, &pNative->jsRecycleBin_, "Heap: PlaylistManager: recycle bin" );
}

const std::string& PlaylistManager::EventIdToType( smp::EventId eventId )
{
    static const std::unordered_map<EventId, std::string> idToType{
        { EventId::kNew_FbPlaylistActivate, "playlistActivate" },
        { EventId::kNew_FbPlaylistCreated, "playlistCreate" },
        { EventId::kNew_FbPlaylistItemEnsureVisible, "ensureTrackVisible" },
        { EventId::kNew_FbPlaylistItemFocusChange, "trackFocusChange" },
        { EventId::kNew_FbPlaylistItemsAdded, "tracksAdd" },
        { EventId::kNew_FbPlaylistItemsRemoved, "tracksRemove" },
        { EventId::kNew_FbPlaylistItemsReordered, "tracksReorder" },
        { EventId::kNew_FbPlaylistItemsReplaced, "tracksReplace" },
        { EventId::kNew_FbPlaylistItemsModified, "tracksInfoChange" },
        { EventId::kNew_FbPlaylistItemsSelectionChange, "tracksSelectionChange" },
        { EventId::kNew_FbPlaylistLocked, "playlistLockChange" },
        { EventId::kNew_FbPlaylistRenamed, "playlistRename" },
        { EventId::kNew_FbPlaylistsRemoved, "playlistsDelete" },
        { EventId::kNew_FbPlaylistsReorder, "playlistsReorder" },
    };

    assert( idToType.contains( eventId ) );
    return idToType.at( eventId );
}

EventStatus PlaylistManager::HandleEvent( JS::HandleObject self, const smp::EventBase& event )
{
    EventStatus status;

    const auto& eventType = EventIdToType( event.GetId() );
    if ( !HasEventListener( eventType ) )
    {
        return status;
    }
    status.isHandled = true;

    JS::RootedObject jsEvent( pJsCtx_, GenerateEvent( event, eventType ) );
    JS::RootedValue jsEventValue( pJsCtx_, JS::ObjectValue( *jsEvent ) );
    DispatchEvent( self, jsEventValue );

    const auto pNativeEvent = convert::to_native::ToValue<JsEvent*>( pJsCtx_, jsEvent );
    status.isDefaultSuppressed = pNativeEvent->get_DefaultPrevented();
    return status;
}

JSObject* PlaylistManager::GenerateEvent( const smp::EventBase& event, const qwr::u8string& eventType )
{
    JS::RootedObject jsEvent( pJsCtx_ );

    switch ( event.GetId() )
    {
    case EventId::kNew_FbPlaylistCreated:
    case EventId::kNew_FbPlaylistItemFocusChange:
    case EventId::kNew_FbPlaylistLocked:
    case EventId::kNew_FbPlaylistRenamed:
    {
        const auto& playlistEvent = static_cast<const smp::PlaylistEvent&>( event );
        jsEvent = mozjs::JsObjectBase<mozjs::PlaylistEvent>::CreateJs(
            pJsCtx_,
            eventType,
            GeneratePlaylistEventProps( playlistEvent ) );
        break;
    }
    case EventId::kNew_FbPlaylistItemEnsureVisible:
    {
        const auto& playlistEvent = static_cast<const smp::PlaylistItemEvent&>( event );
        jsEvent = mozjs::JsObjectBase<mozjs::PlaylistTrackEvent>::CreateJs(
            pJsCtx_,
            eventType,
            GenerateTrackEventProps( playlistEvent ) );
        break;
    }
    case EventId::kNew_FbPlaylistItemsAdded:
    case EventId::kNew_FbPlaylistItemsRemoved:
    case EventId::kNew_FbPlaylistItemsReordered:
    case EventId::kNew_FbPlaylistItemsReplaced:
    case EventId::kNew_FbPlaylistItemsModified:
    case EventId::kNew_FbPlaylistItemsSelectionChange:
    {
        const auto& playlistEvent = static_cast<const smp::PlaylistMultiItemEvent&>( event );
        jsEvent = mozjs::JsObjectBase<mozjs::PlaylistMultiTrackEvent>::CreateJs(
            pJsCtx_,
            eventType,
            GenerateMultiTrackEventProps( playlistEvent ) );
        break;
    }
    case EventId::kNew_FbPlaylistsRemoved:
    case EventId::kNew_FbPlaylistsReorder:
    {
        const auto& playlistEvent = static_cast<const smp::MultiPlaylistEvent&>( event );
        jsEvent = mozjs::JsObjectBase<mozjs::MultiPlaylistEvent>::CreateJs(
            pJsCtx_,
            eventType,
            GenerateMultiPlaylistEventProps( playlistEvent ) );
        break;
    }
    default:
        jsEvent = mozjs::JsEvent::CreateJs( pJsCtx_, eventType, JsEvent::EventProperties{ .cancelable = false } );
        break;
    }

    return jsEvent;
}

JSObject* PlaylistManager::CreateAutoPlaylist( const qwr::u8string& query, const qwr::u8string& sortQuery, uint32_t playlistIndex, const qwr::u8string& name, bool enforceSort )
{
    const uint32_t actualPlaylistIndex = CreatePlaylistImpl( playlistIndex, name );
    assert( pfc_infinite != actualPlaylistIndex );

    try
    {
        autoplaylist_manager::get()->add_client_simple( query.c_str(), sortQuery.c_str(), actualPlaylistIndex, enforceSort ? autoplaylist_flag_sort : 0 );
        smp::PlaylistIndexManager::Get().OnPlaylistAdded( actualPlaylistIndex );
        return GetPlaylist( actualPlaylistIndex );
    }
    catch ( const exception_autoplaylist& e )
    { // Bad query expression
        playlist_manager::get()->remove_playlist( actualPlaylistIndex );
        throw qwr::QwrException( e.what() );
    }
}

JSObject* PlaylistManager::CreateAutoPlaylistWithOpt( size_t optArgCount, const qwr::u8string& query, const qwr::u8string& sortQuery, uint32_t playlistIndex, const qwr::u8string& name, bool enforceSort )
{
    switch ( optArgCount )
    {
    case 0:
        return CreateAutoPlaylist( query, sortQuery, playlistIndex, name, enforceSort );
    case 1:
        return CreateAutoPlaylist( query, sortQuery, playlistIndex, name );
    case 2:
        return CreateAutoPlaylist( query, sortQuery, playlistIndex );
    case 3:
        return CreateAutoPlaylist( query, sortQuery );
    case 4:
        return CreateAutoPlaylist( query );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

JSObject* PlaylistManager::CreatePlaylist( uint32_t playlistIndex, const qwr::u8string& name )
{
    const auto actualPlaylistIndex = CreatePlaylistImpl( playlistIndex, name );
    smp::PlaylistIndexManager::Get().OnPlaylistAdded( actualPlaylistIndex );
    return GetPlaylist( actualPlaylistIndex );
}

JSObject* PlaylistManager::CreatePlaylistWithOpt( size_t optArgCount, uint32_t playlistIndex, const qwr::u8string& name )
{
    switch ( optArgCount )
    {
    case 0:
        return CreatePlaylist( playlistIndex, name );
    case 1:
        return CreatePlaylist( playlistIndex );
    case 2:
        return CreatePlaylist();
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void PlaylistManager::DeletePlaylist( uint32_t playlistIndex, bool switchIfActive )
{
    const auto api = playlist_manager::get();
    qwr::QwrException::ExpectTrue( playlistIndex < api->get_playlist_count(), "Index is out of bounds" );

    if ( switchIfActive )
    {
        api->remove_playlist_switch( playlistIndex );
    }
    else
    {
        api->remove_playlist( playlistIndex );
    }
    smp::PlaylistIndexManager::Get().OnPlaylistRemoved();
}

void PlaylistManager::DeletePlaylistWithOpt( size_t optArgCount, uint32_t playlistIndex, bool switchIfActive )
{
    switch ( optArgCount )
    {
    case 0:
        return DeletePlaylist( playlistIndex, switchIfActive );
    case 1:
        return DeletePlaylist( playlistIndex );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

uint32_t PlaylistManager::DuplicatePlaylist( uint32_t playlistIndexFrom, uint32_t playlistIndexTo, const pfc::string8_fast& name )
{
    auto api = playlist_manager_v4::get();
    qwr::QwrException::ExpectTrue( playlistIndexFrom < api->get_playlist_count(), "Index is out of bounds" );

    metadb_handle_list handles;
    api->playlist_get_all_items( playlistIndexFrom, handles );

    auto newName = name;
    if ( newName.is_empty() )
    {
        (void)api->playlist_get_name( playlistIndexFrom, newName );
    }

    stream_reader_dummy dummy_reader;
    const uint32_t actualPlaylistIndex = api->create_playlist_ex( newName.c_str(), newName.length(), playlistIndexTo, handles, &dummy_reader, fb2k::mainAborter() );
    assert( pfc_infinite != actualPlaylistIndex );

    smp::PlaylistIndexManager::Get().OnPlaylistAdded( actualPlaylistIndex );
    return actualPlaylistIndex;
}

uint32_t PlaylistManager::DuplicatePlaylistWithOpt( size_t optArgCount, uint32_t playlistIndexFrom, uint32_t playlistIndexTo, const pfc::string8_fast& name )
{
    switch ( optArgCount )
    {
    case 0:
        return DuplicatePlaylist( playlistIndexFrom, playlistIndexTo, name );
    case 1:
        return DuplicatePlaylist( playlistIndexFrom, playlistIndexTo );
    case 2:
        return DuplicatePlaylist( playlistIndexFrom, playlistIndexFrom + 1 );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

JSObject* PlaylistManager::FindPlaylist( const qwr::u8string& name ) const
{
    const auto api = playlist_manager::get();
    auto index = api->find_playlist( name.c_str() );
    if ( index < 0 )
    {
        return nullptr;
    }

    return GetPlaylist( index );
}

JSObject* PlaylistManager::GetActivePlaylist() const
{
    const auto api = playlist_manager::get();
    const auto index = api->get_active_playlist();
    if ( index < 0 )
    {
        return nullptr;
    }

    return GetPlaylist( index );
}

JSObject* PlaylistManager::GetCurrentlyPlayingPlaylist() const
{
    const auto api = playlist_manager::get();
    const auto index = api->get_playing_playlist();
    if ( index < 0 )
    {
        return nullptr;
    }

    return GetPlaylist( index );
}

JSObject* PlaylistManager::GetCurrentlyPlayingTrackLocation() const
{
    size_t playlistIndex = 0;
    size_t trackIndex = 0;
    const auto isValid = playlist_manager::get()->get_playing_item_location( &playlistIndex, &trackIndex );
    if ( !isValid )
    {
        return nullptr;
    }

    JS::RootedObject jsObject( pJsCtx_, JS_NewPlainObject( pJsCtx_ ) );
    JsException::ExpectTrue( jsObject );

    JS::RootedObject jsPlaylist( pJsCtx_, GetPlaylist( playlistIndex ) );
    utils::SetProperty( pJsCtx_, jsObject, "playlist", static_cast<JS::HandleObject>( jsPlaylist ) );
    utils::SetProperty( pJsCtx_, jsObject, "trackIndex", trackIndex );
    return jsObject;
}

JSObject* PlaylistManager::GetPlaylist( uint32_t playlistIndex ) const
{
    const auto api = playlist_manager::get();
    qwr::QwrException::ExpectTrue( playlistIndex < api->get_playlist_count(), "Index is out of bounds" );

    const auto id = smp::PlaylistIndexManager::Get().GetId( playlistIndex );
    if ( idToPlaylist_.contains( id ) )
    {
        return idToPlaylist_.at( id ).get();
    }

    auto [it, isEmplaced] = idToPlaylist_.try_emplace( id, Playlist::CreateJs( pJsCtx_, playlistIndex ) );
    return it->second.get();
}

uint32_t PlaylistManager::GetPlaylistCount() const
{
    return playlist_manager::get()->get_playlist_count();
}

void PlaylistManager::MovePlaylist( uint32_t playlistIndexFrom, uint32_t playlistIndexTo )
{
    const auto api = playlist_manager::get();
    qwr::QwrException::ExpectTrue( playlistIndexFrom < api->get_playlist_count(), "Index is out of bounds" );

    const auto playlistCount = api->get_playlist_count();
    if ( playlistIndexTo >= playlistCount )
    {
        playlistIndexTo = playlistCount - 1;
    }
    if ( playlistIndexFrom == playlistIndexTo )
    { // Nothing to do here
        return;
    }

    auto order = ranges::views::indices( playlistCount ) | ranges::to_vector;
    const int8_t inc = ( playlistIndexFrom < playlistIndexTo ? 1 : -1 );
    for ( auto i = playlistIndexFrom; i != playlistIndexTo; i += inc )
    {
        order[i] = i + inc;
    }
    order[playlistIndexTo] = playlistIndexFrom;

    api->reorder( order.data(), order.size() );

    const auto order32 = order | ranges::to<std::vector<uint32_t>>();
    smp::PlaylistIndexManager::Get().OnPlaylistsReordered( order32 );
}

void PlaylistManager::OrderPlaylistsByName( int8_t direction )
{
    const auto api = playlist_manager::get();

    const auto trackCount = api->get_playlist_count();

    std::vector<smp::utils::StrCmpLogicalCmpData> data;
    data.reserve( trackCount );

    pfc::string8_fastalloc buffer;
    buffer.prealloc( 512 );
    for ( auto i: ranges::views::indices( trackCount ) )
    {
        api->playlist_get_name( i, buffer );
        data.emplace_back( qwr::u8string_view{ buffer.c_str(), buffer.length() }, i );
    }

    ranges::actions::sort( data, direction > 0 ? smp::utils::StrCmpLogicalCmp<1> : smp::utils::StrCmpLogicalCmp<-1> );

    const auto order = data | ranges::views::transform( []( const auto& elem ) { return elem.index; } ) | ranges::to_vector;
    api->reorder( order.data(), order.size() );

    const auto order32 = order | ranges::to<std::vector<uint32_t>>();
    smp::PlaylistIndexManager::Get().OnPlaylistsReordered( order32 );
}

void PlaylistManager::OrderPlaylistsByNameWithOpt( size_t optArgCount, int8_t direction )
{
    switch ( optArgCount )
    {
    case 0:
        return OrderPlaylistsByName( direction );
    case 1:
        return OrderPlaylistsByName();
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void PlaylistManager::SetActivePlaylistAsUiEditContext()
{
    ui_edit_context_manager::get()->set_context_active_playlist();
}

JSObject* PlaylistManager::get_RecycleBin() const
{
    if ( !jsRecycleBin_ )
    {
        jsRecycleBin_ = PlaylistRecycleBin::CreateJs( pJsCtx_ );
    }
    return jsRecycleBin_.get();
}

uint32_t PlaylistManager::CreatePlaylistImpl( uint32_t playlistIndex, const qwr::u8string& name )
{
    auto api = playlist_manager::get();

    uint32_t actualPlaylistIndex;
    if ( name.empty() )
    {
        actualPlaylistIndex = api->create_playlist_autoname( playlistIndex );
    }
    else
    {
        actualPlaylistIndex = api->create_playlist( name.c_str(), name.length(), playlistIndex );
    }
    assert( pfc_infinite != actualPlaylistIndex );

    return actualPlaylistIndex;
}

} // namespace mozjs
