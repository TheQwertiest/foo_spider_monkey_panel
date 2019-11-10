#include <stdafx.h>
#include "fb_playlist_manager.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_metadb_handle.h>
#include <js_objects/fb_metadb_handle_list.h>
#include <js_objects/fb_playing_item_location.h>
#include <js_objects/fb_playback_queue_item.h>
#include <js_objects/fb_playlist_recycler.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <utils/string_helpers.h>
#include <utils/text_helpers.h>
#include <utils/location_processor.h>

#include <abort_callback.h>

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
    JsFbPlaylistManager::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "FbPlaylistManager",
    DefaultClassFlags(),
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( AddItemToPlaybackQueue, JsFbPlaylistManager::AddItemToPlaybackQueue );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( AddLocations, JsFbPlaylistManager::AddLocations, JsFbPlaylistManager::AddLocationsWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE( AddPlaylistItemToPlaybackQueue, JsFbPlaylistManager::AddPlaylistItemToPlaybackQueue );
MJS_DEFINE_JS_FN_FROM_NATIVE( ClearPlaylist, JsFbPlaylistManager::ClearPlaylist );
MJS_DEFINE_JS_FN_FROM_NATIVE( ClearPlaylistSelection, JsFbPlaylistManager::ClearPlaylistSelection );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( CreateAutoPlaylist, JsFbPlaylistManager::CreateAutoPlaylist, JsFbPlaylistManager::CreateAutoPlaylistWithOpt, 2 );
MJS_DEFINE_JS_FN_FROM_NATIVE( CreatePlaylist, JsFbPlaylistManager::CreatePlaylist );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( DuplicatePlaylist, JsFbPlaylistManager::DuplicatePlaylist, JsFbPlaylistManager::DuplicatePlaylistWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE( EnsurePlaylistItemVisible, JsFbPlaylistManager::EnsurePlaylistItemVisible );
MJS_DEFINE_JS_FN_FROM_NATIVE( ExecutePlaylistDefaultAction, JsFbPlaylistManager::ExecutePlaylistDefaultAction );
MJS_DEFINE_JS_FN_FROM_NATIVE( FindOrCreatePlaylist, JsFbPlaylistManager::FindOrCreatePlaylist );
MJS_DEFINE_JS_FN_FROM_NATIVE( FindPlaybackQueueItemIndex, JsFbPlaylistManager::FindPlaybackQueueItemIndex );
MJS_DEFINE_JS_FN_FROM_NATIVE( FindPlaylist, JsFbPlaylistManager::FindPlaylist );
MJS_DEFINE_JS_FN_FROM_NATIVE( FlushPlaybackQueue, JsFbPlaylistManager::FlushPlaybackQueue );
MJS_DEFINE_JS_FN_FROM_NATIVE( GetPlaybackQueueContents, JsFbPlaylistManager::GetPlaybackQueueContents );
MJS_DEFINE_JS_FN_FROM_NATIVE( GetPlaybackQueueHandles, JsFbPlaylistManager::GetPlaybackQueueHandles );
MJS_DEFINE_JS_FN_FROM_NATIVE( GetPlayingItemLocation, JsFbPlaylistManager::GetPlayingItemLocation );
MJS_DEFINE_JS_FN_FROM_NATIVE( GetPlaylistFocusItemIndex, JsFbPlaylistManager::GetPlaylistFocusItemIndex );
MJS_DEFINE_JS_FN_FROM_NATIVE( GetPlaylistItems, JsFbPlaylistManager::GetPlaylistItems );
MJS_DEFINE_JS_FN_FROM_NATIVE( GetPlaylistName, JsFbPlaylistManager::GetPlaylistName );
MJS_DEFINE_JS_FN_FROM_NATIVE( GetPlaylistSelectedItems, JsFbPlaylistManager::GetPlaylistSelectedItems );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( InsertPlaylistItems, JsFbPlaylistManager::InsertPlaylistItems, JsFbPlaylistManager::InsertPlaylistItemsWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( InsertPlaylistItemsFilter, JsFbPlaylistManager::InsertPlaylistItemsFilter, JsFbPlaylistManager::InsertPlaylistItemsFilterWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE( IsAutoPlaylist, JsFbPlaylistManager::IsAutoPlaylist );
MJS_DEFINE_JS_FN_FROM_NATIVE( IsPlaylistItemSelected, JsFbPlaylistManager::IsPlaylistItemSelected );
MJS_DEFINE_JS_FN_FROM_NATIVE( IsPlaylistLocked, JsFbPlaylistManager::IsPlaylistLocked );
MJS_DEFINE_JS_FN_FROM_NATIVE( MovePlaylist, JsFbPlaylistManager::MovePlaylist );
MJS_DEFINE_JS_FN_FROM_NATIVE( MovePlaylistSelection, JsFbPlaylistManager::MovePlaylistSelection );
MJS_DEFINE_JS_FN_FROM_NATIVE( PlaylistItemCount, JsFbPlaylistManager::PlaylistItemCount );
MJS_DEFINE_JS_FN_FROM_NATIVE( RemoveItemFromPlaybackQueue, JsFbPlaylistManager::RemoveItemFromPlaybackQueue );
MJS_DEFINE_JS_FN_FROM_NATIVE( RemoveItemsFromPlaybackQueue, JsFbPlaylistManager::RemoveItemsFromPlaybackQueue );
MJS_DEFINE_JS_FN_FROM_NATIVE( RemovePlaylist, JsFbPlaylistManager::RemovePlaylist );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( RemovePlaylistSelection, JsFbPlaylistManager::RemovePlaylistSelection, JsFbPlaylistManager::RemovePlaylistSelectionWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE( RemovePlaylistSwitch, JsFbPlaylistManager::RemovePlaylistSwitch );
MJS_DEFINE_JS_FN_FROM_NATIVE( RenamePlaylist, JsFbPlaylistManager::RenamePlaylist );
MJS_DEFINE_JS_FN_FROM_NATIVE( SetActivePlaylistContext, JsFbPlaylistManager::SetActivePlaylistContext );
MJS_DEFINE_JS_FN_FROM_NATIVE( SetPlaylistFocusItem, JsFbPlaylistManager::SetPlaylistFocusItem );
MJS_DEFINE_JS_FN_FROM_NATIVE( SetPlaylistFocusItemByHandle, JsFbPlaylistManager::SetPlaylistFocusItemByHandle );
MJS_DEFINE_JS_FN_FROM_NATIVE( SetPlaylistSelection, JsFbPlaylistManager::SetPlaylistSelection );
MJS_DEFINE_JS_FN_FROM_NATIVE( SetPlaylistSelectionSingle, JsFbPlaylistManager::SetPlaylistSelectionSingle );
MJS_DEFINE_JS_FN_FROM_NATIVE( ShowAutoPlaylistUI, JsFbPlaylistManager::ShowAutoPlaylistUI );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( SortByFormat, JsFbPlaylistManager::SortByFormat, JsFbPlaylistManager::SortByFormatWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( SortByFormatV2, JsFbPlaylistManager::SortByFormatV2, JsFbPlaylistManager::SortByFormatV2WithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( SortPlaylistsByName, JsFbPlaylistManager::SortPlaylistsByName, JsFbPlaylistManager::SortPlaylistsByNameWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE( UndoBackup, JsFbPlaylistManager::UndoBackup );

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "AddItemToPlaybackQueue", AddItemToPlaybackQueue, 1, DefaultPropsFlags() ),
    JS_FN( "AddLocations", AddLocations, 2, DefaultPropsFlags() ),
    JS_FN( "AddPlaylistItemToPlaybackQueue", AddPlaylistItemToPlaybackQueue, 2, DefaultPropsFlags() ),
    JS_FN( "ClearPlaylist", ClearPlaylist, 1, DefaultPropsFlags() ),
    JS_FN( "ClearPlaylistSelection", ClearPlaylistSelection, 1, DefaultPropsFlags() ),
    JS_FN( "CreateAutoPlaylist", CreateAutoPlaylist, 3, DefaultPropsFlags() ),
    JS_FN( "CreatePlaylist", CreatePlaylist, 2, DefaultPropsFlags() ),
    JS_FN( "DuplicatePlaylist", DuplicatePlaylist, 1, DefaultPropsFlags() ),
    JS_FN( "EnsurePlaylistItemVisible", EnsurePlaylistItemVisible, 2, DefaultPropsFlags() ),
    JS_FN( "ExecutePlaylistDefaultAction", ExecutePlaylistDefaultAction, 2, DefaultPropsFlags() ),
    JS_FN( "FindOrCreatePlaylist", FindOrCreatePlaylist, 2, DefaultPropsFlags() ),
    JS_FN( "FindPlaybackQueueItemIndex", FindPlaybackQueueItemIndex, 3, DefaultPropsFlags() ),
    JS_FN( "FindPlaylist", FindPlaylist, 1, DefaultPropsFlags() ),
    JS_FN( "FlushPlaybackQueue", FlushPlaybackQueue, 0, DefaultPropsFlags() ),
    JS_FN( "GetPlaybackQueueContents", GetPlaybackQueueContents, 0, DefaultPropsFlags() ),
    JS_FN( "GetPlaybackQueueHandles", GetPlaybackQueueHandles, 0, DefaultPropsFlags() ),
    JS_FN( "GetPlayingItemLocation", GetPlayingItemLocation, 0, DefaultPropsFlags() ),
    JS_FN( "GetPlaylistFocusItemIndex", GetPlaylistFocusItemIndex, 1, DefaultPropsFlags() ),
    JS_FN( "GetPlaylistItems", GetPlaylistItems, 1, DefaultPropsFlags() ),
    JS_FN( "GetPlaylistName", GetPlaylistName, 1, DefaultPropsFlags() ),
    JS_FN( "GetPlaylistSelectedItems", GetPlaylistSelectedItems, 1, DefaultPropsFlags() ),
    JS_FN( "InsertPlaylistItems", InsertPlaylistItems, 3, DefaultPropsFlags() ),
    JS_FN( "InsertPlaylistItemsFilter", InsertPlaylistItemsFilter, 3, DefaultPropsFlags() ),
    JS_FN( "IsAutoPlaylist", IsAutoPlaylist, 1, DefaultPropsFlags() ),
    JS_FN( "IsPlaylistItemSelected", IsPlaylistItemSelected, 2, DefaultPropsFlags() ),
    JS_FN( "IsPlaylistLocked", IsPlaylistLocked, 1, DefaultPropsFlags() ),
    JS_FN( "MovePlaylist", MovePlaylist, 2, DefaultPropsFlags() ),
    JS_FN( "MovePlaylistSelection", MovePlaylistSelection, 2, DefaultPropsFlags() ),
    JS_FN( "PlaylistItemCount", PlaylistItemCount, 1, DefaultPropsFlags() ),
    JS_FN( "RemoveItemFromPlaybackQueue", RemoveItemFromPlaybackQueue, 1, DefaultPropsFlags() ),
    JS_FN( "RemoveItemsFromPlaybackQueue", RemoveItemsFromPlaybackQueue, 1, DefaultPropsFlags() ),
    JS_FN( "RemovePlaylist", RemovePlaylist, 1, DefaultPropsFlags() ),
    JS_FN( "RemovePlaylistSelection", RemovePlaylistSelection, 1, DefaultPropsFlags() ),
    JS_FN( "RemovePlaylistSwitch", RemovePlaylistSwitch, 1, DefaultPropsFlags() ),
    JS_FN( "RenamePlaylist", RenamePlaylist, 2, DefaultPropsFlags() ),
    JS_FN( "SetActivePlaylistContext", SetActivePlaylistContext, 0, DefaultPropsFlags() ),
    JS_FN( "SetPlaylistFocusItem", SetPlaylistFocusItem, 2, DefaultPropsFlags() ),
    JS_FN( "SetPlaylistFocusItemByHandle", SetPlaylistFocusItemByHandle, 2, DefaultPropsFlags() ),
    JS_FN( "SetPlaylistSelection", SetPlaylistSelection, 3, DefaultPropsFlags() ),
    JS_FN( "SetPlaylistSelectionSingle", SetPlaylistSelectionSingle, 3, DefaultPropsFlags() ),
    JS_FN( "ShowAutoPlaylistUI", ShowAutoPlaylistUI, 1, DefaultPropsFlags() ),
    JS_FN( "SortByFormat", SortByFormat, 2, DefaultPropsFlags() ),
    JS_FN( "SortByFormatV2", SortByFormatV2, 2, DefaultPropsFlags() ),
    JS_FN( "SortPlaylistsByName", SortPlaylistsByName, 0, DefaultPropsFlags() ),
    JS_FN( "UndoBackup", UndoBackup, 1, DefaultPropsFlags() ),
    JS_FS_END
};

MJS_DEFINE_JS_FN_FROM_NATIVE( get_ActivePlaylist, JsFbPlaylistManager::get_ActivePlaylist );
MJS_DEFINE_JS_FN_FROM_NATIVE( get_PlaybackOrder, JsFbPlaylistManager::get_PlaybackOrder );
MJS_DEFINE_JS_FN_FROM_NATIVE( get_PlayingPlaylist, JsFbPlaylistManager::get_PlayingPlaylist );
MJS_DEFINE_JS_FN_FROM_NATIVE( get_PlaylistCount, JsFbPlaylistManager::get_PlaylistCount );
MJS_DEFINE_JS_FN_FROM_NATIVE( get_PlaylistRecycler, JsFbPlaylistManager::get_PlaylistRecycler );
MJS_DEFINE_JS_FN_FROM_NATIVE( put_ActivePlaylist, JsFbPlaylistManager::put_ActivePlaylist );
MJS_DEFINE_JS_FN_FROM_NATIVE( put_PlaybackOrder, JsFbPlaylistManager::put_PlaybackOrder );
MJS_DEFINE_JS_FN_FROM_NATIVE( put_PlayingPlaylist, JsFbPlaylistManager::put_PlayingPlaylist );

const JSPropertySpec jsProperties[] = {
    JS_PSGS( "ActivePlaylist", get_ActivePlaylist, put_ActivePlaylist, DefaultPropsFlags() ),
    JS_PSGS( "PlaybackOrder", get_PlaybackOrder, put_PlaybackOrder, DefaultPropsFlags() ),
    JS_PSGS( "PlayingPlaylist", get_PlayingPlaylist, put_PlayingPlaylist, DefaultPropsFlags() ),
    JS_PSG( "PlaylistCount", get_PlaylistCount, DefaultPropsFlags() ),
    JS_PSG( "PlaylistRecycler", get_PlaylistRecycler, DefaultPropsFlags() ),
    JS_PS_END
};

} // namespace

namespace mozjs
{

const JSClass JsFbPlaylistManager::JsClass = jsClass;
const JSFunctionSpec* JsFbPlaylistManager::JsFunctions = jsFunctions;
const JSPropertySpec* JsFbPlaylistManager::JsProperties = jsProperties;

JsFbPlaylistManager::JsFbPlaylistManager( JSContext* cx )
    : pJsCtx_( cx )
{
}

JsFbPlaylistManager::~JsFbPlaylistManager()
{
    PrepareForGc();
}

std::unique_ptr<JsFbPlaylistManager>
JsFbPlaylistManager::CreateNative( JSContext* cx )
{
    return std::unique_ptr<JsFbPlaylistManager>( new JsFbPlaylistManager( cx ) );
}

size_t JsFbPlaylistManager::GetInternalSize()
{
    return 0;
}

void JsFbPlaylistManager::PrepareForGc()
{
    jsPlaylistRecycler_.reset();
}

void JsFbPlaylistManager::AddItemToPlaybackQueue( JsFbMetadbHandle* handle )
{
    SmpException::ExpectTrue( handle, "handle argument is null" );

    playlist_manager::get()->queue_add_item( handle->GetHandle() );
}

void JsFbPlaylistManager::AddLocations( uint32_t playlistIndex, JS::HandleValue locations, bool select )
{
    pfc::string_list_impl location_list;
    convert::to_native::ProcessArray<std::u8string>(
        pJsCtx_,
        locations,
        [&location_list]( const auto& location ) { location_list.add_item( location.c_str() ); } );

    const t_size base = playlist_manager::get()->playlist_get_item_count( playlistIndex );
    playlist_incoming_item_filter_v2::get()->process_locations_async(
        location_list,
        playlist_incoming_item_filter_v2::op_flag_no_filter | playlist_incoming_item_filter_v2::op_flag_delay_ui,
        nullptr,
        nullptr,
        nullptr,
        fb2k::service_new<smp::utils::js_process_locations>( playlistIndex, base, select ) );
}

void JsFbPlaylistManager::AddLocationsWithOpt( size_t optArgCount, uint32_t playlistIndex, JS::HandleValue locations, bool select )
{
    switch ( optArgCount )
    {
    case 0:
        return AddLocations( playlistIndex, locations, select );
    case 1:
        return AddLocations( playlistIndex, locations );
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

void JsFbPlaylistManager::AddPlaylistItemToPlaybackQueue( uint32_t playlistIndex, uint32_t playlistItemIndex )
{
    playlist_manager::get()->queue_add_item_playlist( playlistIndex, playlistItemIndex );
}

void JsFbPlaylistManager::ClearPlaylist( uint32_t playlistIndex )
{
    playlist_manager::get()->playlist_clear( playlistIndex );
}

void JsFbPlaylistManager::ClearPlaylistSelection( uint32_t playlistIndex )
{
    playlist_manager::get()->playlist_clear_selection( playlistIndex );
}

uint32_t JsFbPlaylistManager::CreateAutoPlaylist( uint32_t playlistIndex, const std::u8string& name, const std::u8string& query, const std::u8string& sort, uint32_t flags )
{
    const uint32_t upos = CreatePlaylist( playlistIndex, name );
    assert( pfc_infinite != upos );

    try
    {
        autoplaylist_manager::get()->add_client_simple( query.c_str(), sort.c_str(), upos, flags );
        return upos;
    }
    catch ( const pfc::exception& e )
    { // Bad query expression
        playlist_manager::get()->remove_playlist( upos );
        throw SmpException( e.what() );
    }
}

uint32_t JsFbPlaylistManager::CreateAutoPlaylistWithOpt( size_t optArgCount, uint32_t playlistIndex, const std::u8string& name, const std::u8string& query, const std::u8string& sort, uint32_t flags )
{
    switch ( optArgCount )
    {
    case 0:
        return CreateAutoPlaylist( playlistIndex, name, query, sort, flags );
    case 1:
        return CreateAutoPlaylist( playlistIndex, name, query, sort );
    case 2:
        return CreateAutoPlaylist( playlistIndex, name, query );
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

uint32_t JsFbPlaylistManager::CreatePlaylist( uint32_t playlistIndex, const std::u8string& name )
{
    auto api = playlist_manager::get();

    uint32_t upos;
    if ( name.empty() )
    {
        upos = api->create_playlist_autoname( playlistIndex );
    }
    else
    {
        upos = api->create_playlist( name.c_str(), name.length(), playlistIndex );
    }
    assert( pfc_infinite != upos );
    return upos;
}

uint32_t JsFbPlaylistManager::DuplicatePlaylist( uint32_t from, const std::u8string& name )
{
    auto api = playlist_manager_v4::get();

    SmpException::ExpectTrue( from < api->get_playlist_count(), "Index is out of bounds" );

    metadb_handle_list contents;
    api->playlist_get_all_items( from, contents );

    pfc::string8_fast uname = name.c_str();
    if ( uname.is_empty() )
    {
        (void)api->playlist_get_name( from, uname );
    }

    stream_reader_dummy dummy_reader;
    auto& abort = smp::GlobalAbortCallback::GetInstance();
    const uint32_t upos = api->create_playlist_ex( uname.c_str(), uname.length(), from + 1, contents, &dummy_reader, abort );

    assert( pfc_infinite != upos );
    return upos;
}

uint32_t JsFbPlaylistManager::DuplicatePlaylistWithOpt( size_t optArgCount, uint32_t from, const std::u8string& name )
{
    switch ( optArgCount )
    {
    case 0:
        return DuplicatePlaylist( from, name );
    case 1:
        return DuplicatePlaylist( from );
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

void JsFbPlaylistManager::EnsurePlaylistItemVisible( uint32_t playlistIndex, uint32_t playlistItemIndex )
{
    playlist_manager::get()->playlist_ensure_visible( playlistIndex, playlistItemIndex );
}

bool JsFbPlaylistManager::ExecutePlaylistDefaultAction( uint32_t playlistIndex, uint32_t playlistItemIndex )
{
    return playlist_manager::get()->playlist_execute_default_action( playlistIndex, playlistItemIndex );
}

uint32_t JsFbPlaylistManager::FindOrCreatePlaylist( const std::u8string& name, bool unlocked )
{
    auto api = playlist_manager::get();

    uint32_t upos;
    if ( unlocked )
    {
        upos = api->find_or_create_playlist_unlocked( name.c_str(), name.length() );
    }
    else
    {
        upos = api->find_or_create_playlist( name.c_str(), name.length() );
    }

    assert( pfc_infinite != upos );
    return upos;
}

int32_t JsFbPlaylistManager::FindPlaybackQueueItemIndex( JsFbMetadbHandle* handle, uint32_t playlistIndex, uint32_t playlistItemIndex )
{
    SmpException::ExpectTrue( handle, "handle argument is null" );

    t_playback_queue_item item;
    item.m_handle = handle->GetHandle();
    item.m_playlist = playlistIndex;
    item.m_item = playlistItemIndex;

    const uint32_t upos = playlist_manager::get()->queue_find_index( item );
    return ( pfc_infinite == upos ? -1 : static_cast<int32_t>( upos ) );
}

int32_t JsFbPlaylistManager::FindPlaylist( const std::u8string& name )
{
    const uint32_t upos = playlist_manager::get()->find_playlist( name.c_str(), name.length() );
    return ( pfc_infinite == upos ? -1 : static_cast<int32_t>( upos ) );
}

void JsFbPlaylistManager::FlushPlaybackQueue()
{
    playlist_manager::get()->queue_flush();
}

JSObject* JsFbPlaylistManager::GetPlaybackQueueContents()
{
    pfc::list_t<t_playback_queue_item> contents;
    playlist_manager::get()->queue_get_contents( contents );

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue(
        pJsCtx_,
        smp::pfc_x::Make_Stl_CRef( contents ),
        []( const auto& vec, auto index ) {
            return vec[index];
        },
        &jsValue );

    return &jsValue.toObject();
}

JSObject* JsFbPlaylistManager::GetPlaybackQueueHandles()
{
    pfc::list_t<t_playback_queue_item> contents;
    playlist_manager::get()->queue_get_contents( contents );
    
    metadb_handle_list items;
    for ( t_size i = 0, count = contents.get_count(); i < count; ++i )
    {
        items.add_item( contents[i].m_handle );
    }

    return JsFbMetadbHandleList::CreateJs( pJsCtx_, items );
}

JSObject* JsFbPlaylistManager::GetPlayingItemLocation()
{
    t_size playlistIndex = t_size( pfc_infinite );
    t_size playlistItemIndex = t_size( pfc_infinite );
    bool isValid = playlist_manager::get()->get_playing_item_location( &playlistIndex, &playlistItemIndex );

    return JsFbPlayingItemLocation::CreateJs( pJsCtx_, isValid, playlistIndex, playlistItemIndex );
}

int32_t JsFbPlaylistManager::GetPlaylistFocusItemIndex( uint32_t playlistIndex )
{
    const uint32_t upos = playlist_manager::get()->playlist_get_focus_item( playlistIndex );
    return ( pfc_infinite == upos ? -1 : static_cast<int32_t>( upos ) );
}

JSObject* JsFbPlaylistManager::GetPlaylistItems( uint32_t playlistIndex )
{
    metadb_handle_list items;
    playlist_manager::get()->playlist_get_all_items( playlistIndex, items );

    return JsFbMetadbHandleList::CreateJs( pJsCtx_, items );
}

pfc::string8_fast JsFbPlaylistManager::GetPlaylistName( uint32_t playlistIndex )
{
    pfc::string8_fast name;
    playlist_manager::get()->playlist_get_name( playlistIndex, name );
    return name;
}

JSObject* JsFbPlaylistManager::GetPlaylistSelectedItems( uint32_t playlistIndex )
{
    metadb_handle_list items;
    playlist_manager::get()->playlist_get_selected_items( playlistIndex, items );

    return JsFbMetadbHandleList::CreateJs( pJsCtx_, items );
}

void JsFbPlaylistManager::InsertPlaylistItems( uint32_t playlistIndex, uint32_t base, JsFbMetadbHandleList* handles, bool select )
{
    SmpException::ExpectTrue( handles, "handles argument is null" );

    pfc::bit_array_val selection( select );
    playlist_manager::get()->playlist_insert_items( playlistIndex, base, handles->GetHandleList(), selection );
}

void JsFbPlaylistManager::InsertPlaylistItemsWithOpt( size_t optArgCount, uint32_t playlistIndex, uint32_t base, JsFbMetadbHandleList* handles, bool select )
{
    switch ( optArgCount )
    {
    case 0:
        return InsertPlaylistItems( playlistIndex, base, handles, select );
    case 1:
        return InsertPlaylistItems( playlistIndex, base, handles );
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

void JsFbPlaylistManager::InsertPlaylistItemsFilter( uint32_t playlistIndex, uint32_t base, JsFbMetadbHandleList* handles, bool select )
{
    SmpException::ExpectTrue( handles, "handles argument is null" );

    playlist_manager::get()->playlist_insert_items_filter( playlistIndex, base, handles->GetHandleList(), select );
}

void JsFbPlaylistManager::InsertPlaylistItemsFilterWithOpt( size_t optArgCount, uint32_t playlistIndex, uint32_t base, JsFbMetadbHandleList* handles, bool select )
{
    switch ( optArgCount )
    {
    case 0:
        return InsertPlaylistItemsFilter( playlistIndex, base, handles, select );
    case 1:
        return InsertPlaylistItemsFilter( playlistIndex, base, handles );
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

bool JsFbPlaylistManager::IsAutoPlaylist( uint32_t playlistIndex )
{
    SmpException::ExpectTrue( playlistIndex < playlist_manager::get()->get_playlist_count(), "Index is out of bounds" );

    return autoplaylist_manager::get()->is_client_present( playlistIndex );
}

bool JsFbPlaylistManager::IsPlaylistItemSelected( uint32_t playlistIndex, uint32_t playlistItemIndex )
{
    return playlist_manager::get()->playlist_is_item_selected( playlistIndex, playlistItemIndex );
}

bool JsFbPlaylistManager::IsPlaylistLocked( uint32_t playlistIndex )
{
    auto api = playlist_manager::get();
    SmpException::ExpectTrue( playlistIndex < api->get_playlist_count(), "Index is out of bounds" );

    return api->playlist_lock_is_present( playlistIndex );
}

bool JsFbPlaylistManager::MovePlaylist( uint32_t from, uint32_t to )
{
    auto api = playlist_manager::get();
    const t_size playlistCount = api->get_playlist_count();
    if ( from >= playlistCount || to >= playlistCount )
    {
        return false;
    }
    if ( from == to )
    { // Nothing to do here
        return true;
    }

    std::vector<t_size> order = ranges::view::indices( playlistCount );

    const int8_t inc = ( from < to ) ? 1 : -1;
    for ( uint32_t i = from; i != to; i += inc )
    {
        order[i] = i + inc;
    }

    order[to] = from;

    return api->reorder( order.data(), order.size() );
}

bool JsFbPlaylistManager::MovePlaylistSelection( uint32_t playlistIndex, int32_t delta )
{
    return playlist_manager::get()->playlist_move_selection( playlistIndex, delta );
}

uint32_t JsFbPlaylistManager::PlaylistItemCount( uint32_t playlistIndex )
{
    return playlist_manager::get()->playlist_get_item_count( playlistIndex );
}

void JsFbPlaylistManager::RemoveItemFromPlaybackQueue( uint32_t index )
{
    playlist_manager::get()->queue_remove_mask( pfc::bit_array_one( index ) );
}

void JsFbPlaylistManager::RemoveItemsFromPlaybackQueue( JS::HandleValue affectedItems )
{
    auto api = playlist_manager::get();
    pfc::bit_array_bittable affected( api->queue_get_count() );

    convert::to_native::ProcessArray<uint32_t>( pJsCtx_, affectedItems, [&affected]( uint32_t index ) { affected.set( index, true ); } );

    api->queue_remove_mask( affected );
}

bool JsFbPlaylistManager::RemovePlaylist( uint32_t playlistIndex )
{
    return playlist_manager::get()->remove_playlist( playlistIndex );
}

void JsFbPlaylistManager::RemovePlaylistSelection( uint32_t playlistIndex, bool crop )
{
    playlist_manager::get()->playlist_remove_selection( playlistIndex, crop );
}

void JsFbPlaylistManager::RemovePlaylistSelectionWithOpt( size_t optArgCount, uint32_t playlistIndex, bool crop )
{
    switch ( optArgCount )
    {
    case 0:
        return RemovePlaylistSelection( playlistIndex, crop );
    case 1:
        return RemovePlaylistSelection( playlistIndex );
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

bool JsFbPlaylistManager::RemovePlaylistSwitch( uint32_t playlistIndex )
{
    return playlist_manager::get()->remove_playlist_switch( playlistIndex );
}

bool JsFbPlaylistManager::RenamePlaylist( uint32_t playlistIndex, const std::u8string& name )
{
    return playlist_manager::get()->playlist_rename( playlistIndex, name.c_str(), name.length() );
}

void JsFbPlaylistManager::SetActivePlaylistContext()
{
    ui_edit_context_manager::get()->set_context_active_playlist();
}

void JsFbPlaylistManager::SetPlaylistFocusItem( uint32_t playlistIndex, uint32_t playlistItemIndex )
{
    playlist_manager::get()->playlist_set_focus_item( playlistIndex, playlistItemIndex );
}

void JsFbPlaylistManager::SetPlaylistFocusItemByHandle( uint32_t playlistIndex, JsFbMetadbHandle* handle )
{
    SmpException::ExpectTrue( handle, "handle argument is null" );

    playlist_manager::get()->playlist_set_focus_by_handle( playlistIndex, handle->GetHandle() );
}

void JsFbPlaylistManager::SetPlaylistSelection( uint32_t playlistIndex, JS::HandleValue affectedItems, bool state )
{
    auto api = playlist_manager::get();
    pfc::bit_array_bittable affected( api->playlist_get_item_count( playlistIndex ) );

    convert::to_native::ProcessArray<uint32_t>(
        pJsCtx_,
        affectedItems,
        [&affected]( uint32_t index ) { affected.set( index, true ); } );

    pfc::bit_array_val status( state );
    api->playlist_set_selection( playlistIndex, affected, status );
}

void JsFbPlaylistManager::SetPlaylistSelectionSingle( uint32_t playlistIndex, uint32_t playlistItemIndex, bool state )
{
    playlist_manager::get()->playlist_set_selection_single( playlistIndex, playlistItemIndex, state );
}

bool JsFbPlaylistManager::ShowAutoPlaylistUI( uint32_t playlistIndex )
{
    SmpException::ExpectTrue( playlistIndex < playlist_manager::get()->get_playlist_count(), "Index is out of bounds" );

    auto api = autoplaylist_manager::get();
    if ( !api->is_client_present( playlistIndex ) )
    {// TODO v2: replace with error
        return false;
    }

    autoplaylist_client_ptr client = api->query_client( playlistIndex );
    client->show_ui( playlistIndex );

    return true;
}

bool JsFbPlaylistManager::SortByFormat( uint32_t playlistIndex, const std::u8string& pattern, bool selOnly )
{
    return playlist_manager::get()->playlist_sort_by_format( playlistIndex, pattern.empty() ? nullptr : pattern.c_str(), selOnly );
}

bool JsFbPlaylistManager::SortByFormatWithOpt( size_t optArgCount, uint32_t playlistIndex, const std::u8string& pattern, bool selOnly )
{
    switch ( optArgCount )
    {
    case 0:
        return SortByFormat( playlistIndex, pattern, selOnly );
    case 1:
        return SortByFormat( playlistIndex, pattern );
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

bool JsFbPlaylistManager::SortByFormatV2( uint32_t playlistIndex, const std::u8string& pattern, int8_t direction )
{
    auto api = playlist_manager::get();

    metadb_handle_list handles;
    api->playlist_get_all_items( playlistIndex, handles );

    std::vector<t_size> order = ranges::view::indices( handles.get_count() );

    titleformat_object::ptr script;
    titleformat_compiler::get()->compile_safe( script, pattern.c_str() );

    metadb_handle_list_helper::sort_by_format_get_order( handles, order.data(), script, nullptr, direction );

    return api->playlist_reorder_items( playlistIndex, order.data(), order.size() );
}

bool JsFbPlaylistManager::SortByFormatV2WithOpt( size_t optArgCount, uint32_t playlistIndex, const std::u8string& pattern, int8_t direction )
{
    switch ( optArgCount )
    {
    case 0:
        return SortByFormatV2( playlistIndex, pattern, direction );
    case 1:
        return SortByFormatV2( playlistIndex, pattern );
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

void JsFbPlaylistManager::SortPlaylistsByName( int8_t direction )
{
    auto api = playlist_manager::get();
    const size_t count = api->get_playlist_count();

    std::vector<smp::utils::StrCmpLogicalCmpData> data;
    data.reserve( count );

    pfc::string8_fastalloc temp;
    temp.prealloc( 512 );

    for ( size_t i = 0; i < count; ++i )
    {
        api->playlist_get_name( i, temp );
        data.emplace_back( std::u8string_view{ temp.c_str(), temp.length() }, i );
    }

    std::sort( data.begin(), data.end(), (direction > 0 ? smp::utils::StrCmpLogicalCmp<1> : smp::utils::StrCmpLogicalCmp<-1>));

    const std::vector<size_t> order = data | ranges::view::transform( []( const auto& elem ) { return elem.index; } );
    api->reorder( order.data(), order.size() );
}

void JsFbPlaylistManager::SortPlaylistsByNameWithOpt( size_t optArgCount, int8_t direction )
{
    switch ( optArgCount )
    {
    case 0:
        return SortPlaylistsByName( direction );
    case 1:
        return SortPlaylistsByName();
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

void JsFbPlaylistManager::UndoBackup( uint32_t playlistIndex )
{
    playlist_manager::get()->playlist_undo_backup( playlistIndex );
}

int32_t JsFbPlaylistManager::get_ActivePlaylist()
{
    uint32_t upos = playlist_manager::get()->get_active_playlist();
    return ( pfc_infinite == upos ? -1 : static_cast<int32_t>( upos ) );
}

uint32_t JsFbPlaylistManager::get_PlaybackOrder()
{
    return playlist_manager::get()->playback_order_get_active();
}

int32_t JsFbPlaylistManager::get_PlayingPlaylist()
{
    uint32_t upos = playlist_manager::get()->get_playing_playlist();
    return ( pfc_infinite == upos ? -1 : static_cast<int32_t>( upos ) );
}

uint32_t JsFbPlaylistManager::get_PlaylistCount()
{
    return playlist_manager::get()->get_playlist_count();
}

JSObject* JsFbPlaylistManager::get_PlaylistRecycler()
{
    if ( !jsPlaylistRecycler_.initialized() )
    {
        jsPlaylistRecycler_.init( pJsCtx_, JsFbPlaylistRecycler::CreateJs( pJsCtx_ ) );
    }

    return jsPlaylistRecycler_;
}

void JsFbPlaylistManager::put_ActivePlaylist( uint32_t playlistIndex )
{
    auto api = playlist_manager::get();
    SmpException::ExpectTrue( playlistIndex < api->get_playlist_count(), "Index is out of bounds" );

    api->set_active_playlist( playlistIndex );
}

void JsFbPlaylistManager::put_PlaybackOrder( uint32_t order )
{
    auto api = playlist_manager::get();
    SmpException::ExpectTrue( order < api->playback_order_get_count(), "Unknown playback order id: {}", order );

    api->playback_order_set_active( order );
}

void JsFbPlaylistManager::put_PlayingPlaylist( uint32_t playlistIndex )
{
    auto api = playlist_manager::get();
    SmpException::ExpectTrue( playlistIndex < api->get_playlist_count(), "Index is out of bounds" );

    api->set_playing_playlist( playlistIndex );
}

} // namespace mozjs
