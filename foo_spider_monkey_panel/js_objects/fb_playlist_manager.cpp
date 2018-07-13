#include <stdafx.h>
#include "fb_playlist_manager.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_metadb_handle.h>
#include <js_objects/fb_metadb_handle_list.h>
#include <js_objects/fb_playing_item_location.h>
#include <js_objects/fb_playback_queue_item.h>
#include <js_objects/fb_playlist_recycler_manager.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>

#include <helpers.h>

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
    JsFinalizeOp<JsFbPlaylistManager>,
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

MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, AddItemToPlaybackQueue        );
MJS_DEFINE_JS_TO_NATIVE_FN_WITH_OPT( JsFbPlaylistManager, AddLocations, AddLocationsWithOpt, 1 );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, AddPlaylistItemToPlaybackQueue);
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, ClearPlaylist                 );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, ClearPlaylistSelection        );
MJS_DEFINE_JS_TO_NATIVE_FN_WITH_OPT( JsFbPlaylistManager, CreateAutoPlaylist, CreateAutoPlaylistWithOpt, 2 );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, CreatePlaylist                );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, DuplicatePlaylist            );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, EnsurePlaylistItemVisible    );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, ExecutePlaylistDefaultAction  );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, FindOrCreatePlaylist          );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, FindPlaybackQueueItemIndex    );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, FindPlaylist                  );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, FlushPlaybackQueue           );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, GetPlaybackQueueContents   );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, GetPlaybackQueueHandles       );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, GetPlayingItemLocation        );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, GetPlaylistFocusItemIndex     );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, GetPlaylistItems              );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, GetPlaylistName               );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, GetPlaylistSelectedItems      );
MJS_DEFINE_JS_TO_NATIVE_FN_WITH_OPT( JsFbPlaylistManager, InsertPlaylistItems, InsertPlaylistItemsWithOpt, 1 );
MJS_DEFINE_JS_TO_NATIVE_FN_WITH_OPT( JsFbPlaylistManager, InsertPlaylistItemsFilter, InsertPlaylistItemsFilterWithOpt, 1 );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, IsAutoPlaylist                );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, IsPlaylistItemSelected        );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, IsPlaylistLocked              );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, MovePlaylist                  );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, MovePlaylistSelection         );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, PlaylistItemCount             );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, RemoveItemFromPlaybackQueue  );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, RemoveItemsFromPlaybackQueue);
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, RemovePlaylist                );
MJS_DEFINE_JS_TO_NATIVE_FN_WITH_OPT( JsFbPlaylistManager, RemovePlaylistSelection, RemovePlaylistSelectionWithOpt, 1 );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, RemovePlaylistSwitch          );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, RenamePlaylist               );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, SetActivePlaylistContext      );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, SetPlaylistFocusItem          );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, SetPlaylistFocusItemByHandle  );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, SetPlaylistSelection       );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, SetPlaylistSelectionSingle    );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, ShowAutoPlaylistUI            );
MJS_DEFINE_JS_TO_NATIVE_FN_WITH_OPT( JsFbPlaylistManager, SortByFormat, SortByFormatWithOpt, 1 );
MJS_DEFINE_JS_TO_NATIVE_FN_WITH_OPT( JsFbPlaylistManager, SortByFormatV2, SortByFormatV2WithOpt, 1 );
MJS_DEFINE_JS_TO_NATIVE_FN_WITH_OPT( JsFbPlaylistManager, SortPlaylistsByName, SortPlaylistsByNameWithOpt, 1 );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, UndoBackup                    );


const JSFunctionSpec jsFunctions[] = {
    JS_FN("AddItemToPlaybackQueue", AddItemToPlaybackQueue, 1, DefaultPropsFlags() ),
    JS_FN("AddLocations", AddLocations, 2, DefaultPropsFlags() ),
    JS_FN("AddPlaylistItemToPlaybackQueue", AddPlaylistItemToPlaybackQueue, 2, DefaultPropsFlags() ),
    JS_FN("ClearPlaylist", ClearPlaylist, 1, DefaultPropsFlags() ),
    JS_FN("ClearPlaylistSelection", ClearPlaylistSelection, 1, DefaultPropsFlags() ),
    JS_FN("CreateAutoPlaylist", CreateAutoPlaylist, 3, DefaultPropsFlags() ),
    JS_FN("CreatePlaylist", CreatePlaylist, 2, DefaultPropsFlags() ),
    JS_FN("DuplicatePlaylist", DuplicatePlaylist, 2, DefaultPropsFlags() ),
    JS_FN("EnsurePlaylistItemVisible", EnsurePlaylistItemVisible, 2, DefaultPropsFlags() ),
    JS_FN("ExecutePlaylistDefaultAction", ExecutePlaylistDefaultAction, 2, DefaultPropsFlags() ),
    JS_FN("FindOrCreatePlaylist", FindOrCreatePlaylist, 2, DefaultPropsFlags() ),
    JS_FN("FindPlaybackQueueItemIndex", FindPlaybackQueueItemIndex, 3, DefaultPropsFlags() ),
    JS_FN("FindPlaylist", FindPlaylist, 1, DefaultPropsFlags() ),
    JS_FN("FlushPlaybackQueue", FlushPlaybackQueue, 0, DefaultPropsFlags() ),
    JS_FN("GetPlaybackQueueContents", GetPlaybackQueueContents, 0, DefaultPropsFlags() ),
    JS_FN("GetPlaybackQueueHandles", GetPlaybackQueueHandles, 0, DefaultPropsFlags() ),
    JS_FN("GetPlayingItemLocation", GetPlayingItemLocation, 0, DefaultPropsFlags() ),
    JS_FN("GetPlaylistFocusItemIndex", GetPlaylistFocusItemIndex, 1, DefaultPropsFlags() ),
    JS_FN("GetPlaylistItems", GetPlaylistItems, 1, DefaultPropsFlags() ),
    JS_FN("GetPlaylistName", GetPlaylistName, 1, DefaultPropsFlags() ),
    JS_FN("GetPlaylistSelectedItems", GetPlaylistSelectedItems, 1, DefaultPropsFlags() ),
    JS_FN("InsertPlaylistItems", InsertPlaylistItems, 3, DefaultPropsFlags() ),
    JS_FN("InsertPlaylistItemsFilter", InsertPlaylistItemsFilter, 3, DefaultPropsFlags() ),
    JS_FN("IsAutoPlaylist", IsAutoPlaylist, 1, DefaultPropsFlags() ),
    JS_FN("IsPlaylistItemSelected", IsPlaylistItemSelected, 2, DefaultPropsFlags() ),
    JS_FN("IsPlaylistLocked", IsPlaylistLocked, 1, DefaultPropsFlags() ),
    JS_FN("MovePlaylist", MovePlaylist, 2, DefaultPropsFlags() ),
    JS_FN("MovePlaylistSelection", MovePlaylistSelection, 2, DefaultPropsFlags() ),
    JS_FN("PlaylistItemCount", PlaylistItemCount, 1, DefaultPropsFlags() ),
    JS_FN("RemoveItemFromPlaybackQueue", RemoveItemFromPlaybackQueue, 1, DefaultPropsFlags() ),
    JS_FN("RemoveItemsFromPlaybackQueue", RemoveItemsFromPlaybackQueue, 1, DefaultPropsFlags() ),
    JS_FN("RemovePlaylist", RemovePlaylist, 1, DefaultPropsFlags() ),
    JS_FN("RemovePlaylistSelection", RemovePlaylistSelection, 1, DefaultPropsFlags() ),
    JS_FN("RemovePlaylistSwitch", RemovePlaylistSwitch, 1, DefaultPropsFlags() ),
    JS_FN("RenamePlaylist", RenamePlaylist, 2, DefaultPropsFlags() ),
    JS_FN("SetActivePlaylistContext", SetActivePlaylistContext, 0, DefaultPropsFlags() ),
    JS_FN("SetPlaylistFocusItem", SetPlaylistFocusItem, 2, DefaultPropsFlags() ),
    JS_FN("SetPlaylistFocusItemByHandle", SetPlaylistFocusItemByHandle, 2, DefaultPropsFlags() ),
    JS_FN("SetPlaylistSelection", SetPlaylistSelection, 3, DefaultPropsFlags() ),
    JS_FN("SetPlaylistSelectionSingle", SetPlaylistSelectionSingle, 3, DefaultPropsFlags() ),
    JS_FN("ShowAutoPlaylistUI", ShowAutoPlaylistUI, 1, DefaultPropsFlags() ),
    JS_FN("SortByFormat", SortByFormat, 2, DefaultPropsFlags() ),
    JS_FN("SortByFormatV2", SortByFormatV2, 2, DefaultPropsFlags() ),
    JS_FN("SortPlaylistsByName", SortPlaylistsByName, 0, DefaultPropsFlags() ),
    JS_FN("UndoBackup", UndoBackup, 1, DefaultPropsFlags() ),
    JS_FS_END
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, get_ActivePlaylist );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, get_PlaybackOrder );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, get_PlayingPlaylist );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, get_PlaylistCount );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, get_PlaylistRecyclerManager );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, put_ActivePlaylist );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, put_PlaybackOrder );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, put_PlayingPlaylist );

const JSPropertySpec jsProperties[] = {
    JS_PSGS( "ActivePlaylist", get_ActivePlaylist, put_ActivePlaylist, DefaultPropsFlags() ),
    JS_PSGS( "PlaybackOrder", get_PlaybackOrder, put_PlaybackOrder, DefaultPropsFlags() ),
    JS_PSGS( "PlayingPlaylist", get_PlayingPlaylist, put_PlayingPlaylist, DefaultPropsFlags() ),
    JS_PSG( "PlaylistCount", get_PlaylistCount, DefaultPropsFlags() ),
    JS_PSG( "PlaylistRecyclerManager", get_PlaylistRecyclerManager, DefaultPropsFlags() ),
    JS_PS_END
};

}

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
    jsPlaylistRecycler_.reset();
}

std::unique_ptr<JsFbPlaylistManager>
JsFbPlaylistManager::CreateNative( JSContext* cx )
{
    return std::unique_ptr<JsFbPlaylistManager>( new JsFbPlaylistManager( cx ) );
}

std::optional<std::nullptr_t>
JsFbPlaylistManager::AddItemToPlaybackQueue( JsFbMetadbHandle* handle )
{
    if ( !handle )
    {
        JS_ReportErrorUTF8( pJsCtx_, "handle argument is null" );
        return std::nullopt;
    }

    playlist_manager::get()->queue_add_item( handle->GetHandle() );
    return nullptr;
}

std::optional<std::nullptr_t> 
JsFbPlaylistManager::AddLocations( uint32_t playlistIndex, JS::HandleValue locations, bool select )
{ 
    JS::RootedObject jsObject( pJsCtx_, locations.toObjectOrNull() );
    if ( !jsObject )
    {
        JS_ReportErrorUTF8( pJsCtx_, "locations argument is not a JS object" );
        return std::nullopt;
    }

    bool is;
    if ( !JS_IsArrayObject( pJsCtx_, jsObject, &is ) )
    {
        JS_ReportErrorUTF8( pJsCtx_, "locations argument is an array" );
        return std::nullopt;
    }

    uint32_t arraySize;
    if ( !JS_GetArrayLength( pJsCtx_, jsObject, &arraySize ) )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Failed to get locations argument array length" );
        return std::nullopt;
    }

    pfc::string_list_impl locations2;

    JS::RootedValue arrayElement( pJsCtx_ );
    for ( uint32_t i = 0; i < arraySize; ++i )
    {
        if ( !JS_GetElement( pJsCtx_, jsObject, i, &arrayElement ) )
        {
            JS_ReportErrorUTF8( pJsCtx_, "Failed to get locations[%u]", i );
            return std::nullopt;
        }

        bool isValid;
        pfc::string8_fast path( convert::to_native::ToValue<pfc::string8_fast>( pJsCtx_, arrayElement, isValid ) );
        if ( !isValid )
        {
            JS_ReportErrorUTF8( pJsCtx_, "locations[%u] is not a string", i );
            return std::nullopt;
        }

        locations2.add_item( path.c_str() );
    }

    t_size base = playlist_manager::get()->playlist_get_item_count( playlistIndex );
    playlist_incoming_item_filter_v2::get()->process_locations_async(
        locations2,
        playlist_incoming_item_filter_v2::op_flag_no_filter | playlist_incoming_item_filter_v2::op_flag_delay_ui,
        NULL,
        NULL,
        NULL,
        new service_impl_t<helpers::js_process_locations>( playlistIndex, base, select ) );

    return nullptr;
}

std::optional<std::nullptr_t> 
JsFbPlaylistManager::AddLocationsWithOpt( size_t optArgCount, uint32_t playlistIndex, JS::HandleValue locations, bool select )
{
    if ( optArgCount > 1 )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: invalid number of optional arguments specified: %u", optArgCount );
        return std::nullopt;
    }

    if ( optArgCount == 1 )
    {
        return AddLocations( playlistIndex, locations );
    }

    return AddLocations( playlistIndex, locations, select );
}

std::optional<std::nullptr_t>
JsFbPlaylistManager::AddPlaylistItemToPlaybackQueue( uint32_t playlistIndex, uint32_t playlistItemIndex )
{
    playlist_manager::get()->queue_add_item_playlist( playlistIndex, playlistItemIndex );
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbPlaylistManager::ClearPlaylist( uint32_t playlistIndex )
{
    playlist_manager::get()->playlist_clear( playlistIndex );
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbPlaylistManager::ClearPlaylistSelection( uint32_t playlistIndex )
{
    playlist_manager::get()->playlist_clear_selection( playlistIndex );
    return nullptr;
}

std::optional<int32_t>
JsFbPlaylistManager::CreateAutoPlaylist( uint32_t playlistIndex, const pfc::string8_fast& name, const pfc::string8_fast& query, const pfc::string8_fast& sort, uint32_t flags )
{
    std::optional<int32_t> optPos = CreatePlaylist( playlistIndex, name );
    if ( !optPos )
    {
        return std::nullopt;
    }
         
    if (-1 == optPos )
    {// Not a script stopping error
        return -1;        
    }

    uint32_t upos = pfc_infinite;
    try
    {
        autoplaylist_manager::get()->add_client_simple( query.c_str(), sort.c_str(), upos, flags );
    }
    catch ( ... )
    {
        playlist_manager::get()->remove_playlist( upos );
        return -1;
    }
    
    return (pfc_infinite == upos ? -1 : static_cast<int32_t>(upos));
}

std::optional<int32_t> 
JsFbPlaylistManager::CreateAutoPlaylistWithOpt( size_t optArgCount, uint32_t playlistIndex, const pfc::string8_fast& name, const pfc::string8_fast& query, const pfc::string8_fast& sort, uint32_t flags )
{
    if ( optArgCount > 2 )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: invalid number of optional arguments specified: %d", optArgCount );
        return std::nullopt;
    }

    if ( optArgCount == 2 )
    {
        return CreateAutoPlaylist( playlistIndex, name, query );
    }
    else if ( optArgCount == 1 )
    {
        return CreateAutoPlaylist( playlistIndex, name, query, sort );
    }

    return CreateAutoPlaylist( playlistIndex, name, query, sort, flags );
}

std::optional<int32_t>
JsFbPlaylistManager::CreatePlaylist( uint32_t playlistIndex, const pfc::string8_fast& name )
{
    auto api = playlist_manager::get();

    uint32_t upos;
    if ( name.is_empty() )
    {
        upos = api->create_playlist_autoname( playlistIndex );
    }
    else
    {
        upos = api->create_playlist( name.c_str(), name.length(), playlistIndex );
    }
    
    return (pfc_infinite == upos ? -1 : static_cast<int32_t>(upos));
}

std::optional<uint32_t>
JsFbPlaylistManager::DuplicatePlaylist( uint32_t from, const pfc::string8_fast& name )
{
    auto api = playlist_manager_v4::get();

    if ( from >= api->get_playlist_count() )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Index is out of bounds" );
        return std::nullopt;
    }

    metadb_handle_list contents;
    api->playlist_get_all_items( from, contents );

    pfc::string8_fast uname( name.c_str(), name.length() );
    if ( uname.is_empty() )
    {
        api->playlist_get_name( from, uname );
    }

    stream_reader_dummy dummy_reader;
    return api->create_playlist_ex( uname.get_ptr(), uname.get_length(), from + 1, contents, &dummy_reader, abort_callback_dummy() );
}

std::optional<std::nullptr_t>
JsFbPlaylistManager::EnsurePlaylistItemVisible( uint32_t playlistIndex, uint32_t playlistItemIndex )
{
    playlist_manager::get()->playlist_ensure_visible( playlistIndex, playlistItemIndex );
    return nullptr;
}

std::optional<bool>
JsFbPlaylistManager::ExecutePlaylistDefaultAction( uint32_t playlistIndex, uint32_t playlistItemIndex )
{
    return playlist_manager::get()->playlist_execute_default_action( playlistIndex, playlistItemIndex );
}

std::optional<int32_t>
JsFbPlaylistManager::FindOrCreatePlaylist( const pfc::string8_fast& name, bool unlocked )
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

    return (pfc_infinite == upos ? -1 : static_cast<int32_t>(upos));
}

std::optional<int32_t>
JsFbPlaylistManager::FindPlaybackQueueItemIndex( JsFbMetadbHandle* handle, uint32_t playlistIndex, uint32_t playlistItemIndex )
{
    if ( !handle )
    {
        JS_ReportErrorUTF8( pJsCtx_, "handle argument is null" );
        return std::nullopt;
    }

    t_playback_queue_item item;
    item.m_handle = handle->GetHandle();
    item.m_playlist = playlistIndex;
    item.m_item = playlistItemIndex;

    uint32_t upos = playlist_manager::get()->queue_find_index( item );
    return (pfc_infinite == upos ? -1 : static_cast<int32_t>(upos));
}

std::optional<int32_t>
JsFbPlaylistManager::FindPlaylist( const pfc::string8_fast& name )
{
    uint32_t upos = playlist_manager::get()->find_playlist( name.c_str(), name.length() );
    return (pfc_infinite == upos ? -1 : static_cast<int32_t>(upos));
}

std::optional<std::nullptr_t>
JsFbPlaylistManager::FlushPlaybackQueue()
{
    playlist_manager::get()->queue_flush();
    return nullptr;
}

std::optional<JSObject*> 
JsFbPlaylistManager::GetPlaybackQueueContents()
{
    pfc::list_t<t_playback_queue_item> contents;
    playlist_manager::get()->queue_get_contents( contents );
    t_size count = contents.get_count();

    JS::RootedObject jsArray( pJsCtx_, JS_NewArrayObject( pJsCtx_, count ) );
    if ( !jsArray )
    {
        JS_ReportOutOfMemory( pJsCtx_ );
        return std::nullopt;
    }

    JS::RootedValue jsValue( pJsCtx_ );
    JS::RootedObject jsObject( pJsCtx_ );
    for ( t_size i = 0; i < count; ++i )
    {
        jsObject.set( JsFbPlaybackQueueItem::Create( pJsCtx_, contents[i] ) );
        if ( !jsObject )
        {
            JS_ReportErrorUTF8( pJsCtx_, "Internal error: failed to create JS object" );
            return std::nullopt;
        }

        jsValue.set( JS::ObjectValue( *jsObject ) );
        if ( !JS_SetElement( pJsCtx_, jsArray, i, jsValue ) )
        {
            JS_ReportErrorUTF8( pJsCtx_, "Internal error: JS_SetElement failed" );
            return std::nullopt;
        }
    }

    return jsArray;
}

std::optional<JSObject*>
JsFbPlaylistManager::GetPlaybackQueueHandles()
{
    pfc::list_t<t_playback_queue_item> contents;
    playlist_manager::get()->queue_get_contents( contents );
    t_size count = contents.get_count();
    metadb_handle_list items;
    for ( t_size i = 0; i < count; ++i )
    {
        items.add_item( contents[i].m_handle );
    }

    JS::RootedObject jsObject( pJsCtx_, JsFbMetadbHandleList::Create( pJsCtx_, items ) );
    if ( !jsObject )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    return jsObject;
}

std::optional<JSObject*>
JsFbPlaylistManager::GetPlayingItemLocation()
{
    t_size playlistIndex = pfc_infinite;
    t_size playlistItemIndex = pfc_infinite;
    bool isValid = playlist_manager::get()->get_playing_item_location( &playlistIndex, &playlistItemIndex );
    
    JS::RootedObject jsObject( pJsCtx_, JsFbPlayingItemLocation::Create( pJsCtx_, isValid, playlistIndex, playlistItemIndex ) );
    if ( !jsObject )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    return jsObject;
}

std::optional<int32_t>
JsFbPlaylistManager::GetPlaylistFocusItemIndex( uint32_t playlistIndex )
{
    uint32_t upos = playlist_manager::get()->playlist_get_focus_item( playlistIndex );
    return (pfc_infinite == upos ? -1 : static_cast<int32_t>(upos));
}

std::optional<JSObject*>
JsFbPlaylistManager::GetPlaylistItems( uint32_t playlistIndex )
{
    metadb_handle_list items;
    playlist_manager::get()->playlist_get_all_items( playlistIndex, items );
    
    JS::RootedObject jsObject( pJsCtx_, JsFbMetadbHandleList::Create( pJsCtx_, items ) );
    if ( !jsObject )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    return jsObject;
}

std::optional<pfc::string8_fast>
JsFbPlaylistManager::GetPlaylistName( uint32_t playlistIndex )
{
    pfc::string8_fast name;
    playlist_manager::get()->playlist_get_name( playlistIndex, name );
    return name.c_str();
}

std::optional<JSObject*>
JsFbPlaylistManager::GetPlaylistSelectedItems( uint32_t playlistIndex )
{
    metadb_handle_list items;
    playlist_manager::get()->playlist_get_selected_items( playlistIndex, items );
    
    JS::RootedObject jsObject( pJsCtx_, JsFbMetadbHandleList::Create( pJsCtx_, items ) );
    if ( !jsObject )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    return jsObject;
}

std::optional<std::nullptr_t>
JsFbPlaylistManager::InsertPlaylistItems( uint32_t playlistIndex, uint32_t base, JsFbMetadbHandleList* handles, bool select )
{    
    if ( !handles )
    {
        JS_ReportErrorUTF8( pJsCtx_, "handles argument is null" );
        return std::nullopt;
    }

    pfc::bit_array_val selection( select );
    playlist_manager::get()->playlist_insert_items( playlistIndex, base, handles->GetHandleList(), selection );
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbPlaylistManager::InsertPlaylistItemsWithOpt( size_t optArgCount, uint32_t playlistIndex, uint32_t base, JsFbMetadbHandleList* handles, bool select )
{
    if ( optArgCount > 1 )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: invalid number of optional arguments specified: %d", optArgCount );
        return std::nullopt;
    }

    if ( optArgCount == 1 )
    {
        return InsertPlaylistItems( playlistIndex, base, handles );
    }

    return InsertPlaylistItems( playlistIndex, base, handles, select );
}

std::optional<std::nullptr_t>
JsFbPlaylistManager::InsertPlaylistItemsFilter( uint32_t playlistIndex, uint32_t base, JsFbMetadbHandleList* handles, bool select )
{    
    if ( !handles )
    {
        JS_ReportErrorUTF8( pJsCtx_, "handles argument is null" );
        return std::nullopt;
    }

    playlist_manager::get()->playlist_insert_items_filter( playlistIndex, base, handles->GetHandleList(), select );
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbPlaylistManager::InsertPlaylistItemsFilterWithOpt( size_t optArgCount, uint32_t playlistIndex, uint32_t base, JsFbMetadbHandleList* handles, bool select )
{
    if ( optArgCount > 1 )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: invalid number of optional arguments specified: %d", optArgCount );
        return std::nullopt;
    }

    if ( optArgCount == 1 )
    {
        return InsertPlaylistItemsFilter( playlistIndex, base, handles );
    }

    return InsertPlaylistItemsFilter( playlistIndex, base, handles, select );
}

std::optional<bool>
JsFbPlaylistManager::IsAutoPlaylist( uint32_t playlistIndex )
{
    if ( playlistIndex >= playlist_manager::get()->get_playlist_count() )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Index is out of bounds" );
        return std::nullopt;
    }

    return autoplaylist_manager::get()->is_client_present( playlistIndex );
}

std::optional<bool>
JsFbPlaylistManager::IsPlaylistItemSelected( uint32_t playlistIndex, uint32_t playlistItemIndex )
{
    return playlist_manager::get()->playlist_is_item_selected( playlistIndex, playlistItemIndex );
}

std::optional<bool>
JsFbPlaylistManager::IsPlaylistLocked( uint32_t playlistIndex )
{
    auto api = playlist_manager::get();
    if ( playlistIndex >= api->get_playlist_count() )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Index is out of bounds" );
        return std::nullopt;
    }

    return api->playlist_lock_is_present( playlistIndex );
}

std::optional<bool>
JsFbPlaylistManager::MovePlaylist( uint32_t from, uint32_t to )
{
    auto api = playlist_manager::get();
    order_helper order( api->get_playlist_count() );

    if ( from >= order.get_count() || to >= order.get_count() )
    {
        return false;
    }

    int8_t inc = (from < to) ? 1 : -1;
    for ( int32_t i = from; i != to; i += inc )
    {
        order[i] = order[i + inc];
    }

    order[to] = from;

    return api->reorder( order.get_ptr(), order.get_count() );
}

std::optional<bool>
JsFbPlaylistManager::MovePlaylistSelection( uint32_t playlistIndex, int32_t delta )
{
    return playlist_manager::get()->playlist_move_selection( playlistIndex, delta );
}

std::optional<uint32_t>
JsFbPlaylistManager::PlaylistItemCount( uint32_t playlistIndex )
{
    return playlist_manager::get()->playlist_get_item_count( playlistIndex );
}

std::optional<std::nullptr_t>
JsFbPlaylistManager::RemoveItemFromPlaybackQueue( uint32_t index )
{
    playlist_manager::get()->queue_remove_mask( pfc::bit_array_one( index ) );
    return nullptr;
}

std::optional<std::nullptr_t> 
JsFbPlaylistManager::RemoveItemsFromPlaybackQueue( JS::HandleValue affectedItems )
{
    JS::RootedObject jsObject( pJsCtx_, affectedItems.toObjectOrNull() );
    if ( !jsObject )
    {
        JS_ReportErrorUTF8( pJsCtx_, "affectedItems argument is not a JS object" );
        return std::nullopt;
    }

    bool is;
    if ( !JS_IsArrayObject( pJsCtx_, jsObject, &is ) )
    {
        JS_ReportErrorUTF8( pJsCtx_, "affectedItems argument is an array" );
        return std::nullopt;
    }

    uint32_t arraySize;
    if ( !JS_GetArrayLength( pJsCtx_, jsObject, &arraySize ) )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Failed to get affectedItems argument array length" );
        return std::nullopt;
    }

    auto api = playlist_manager::get();
    pfc::bit_array_bittable affected( api->queue_get_count() );
    
    JS::RootedValue arrayElement( pJsCtx_ );
    for ( uint32_t i = 0; i < arraySize; ++i )
    {
        if ( !JS_GetElement( pJsCtx_, jsObject, i, &arrayElement ) )
        {
            JS_ReportErrorUTF8( pJsCtx_, "Failed to get affectedItems[%u]", i );
            return std::nullopt;
        }

        bool isValid;
        uint32_t affectedIdx( convert::to_native::ToValue<uint32_t>( pJsCtx_, arrayElement, isValid ) );
        if ( !isValid )
        {
            JS_ReportErrorUTF8( pJsCtx_, "affectedItems[%u] can't be converted to number" );
            return std::nullopt;
        }

        affected.set( affectedIdx, true );
    }
    
    api->queue_remove_mask( affected );
    return nullptr;
}

std::optional<bool>
JsFbPlaylistManager::RemovePlaylist( uint32_t playlistIndex )
{
    return playlist_manager::get()->remove_playlist( playlistIndex );
}

std::optional<std::nullptr_t>
JsFbPlaylistManager::RemovePlaylistSelection( uint32_t playlistIndex, bool crop )
{
    playlist_manager::get()->playlist_remove_selection( playlistIndex, crop );
    return nullptr;
}

std::optional<std::nullptr_t> 
JsFbPlaylistManager::RemovePlaylistSelectionWithOpt( size_t optArgCount, uint32_t playlistIndex, bool crop )
{
    if ( optArgCount > 1 )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: invalid number of optional arguments specified: %d", optArgCount );
        return std::nullopt;
    }

    if ( optArgCount == 1 )
    {
        return RemovePlaylistSelection( playlistIndex );
    }

    return RemovePlaylistSelection( playlistIndex, crop );
}

std::optional<bool>
JsFbPlaylistManager::RemovePlaylistSwitch( uint32_t playlistIndex )
{
    return playlist_manager::get()->remove_playlist_switch( playlistIndex );
}

std::optional<bool>
JsFbPlaylistManager::RenamePlaylist( uint32_t playlistIndex, const pfc::string8_fast& name )
{
    return playlist_manager::get()->playlist_rename( playlistIndex, name.c_str(), name.length() );
}

std::optional<std::nullptr_t>
JsFbPlaylistManager::SetActivePlaylistContext()
{
    ui_edit_context_manager::get()->set_context_active_playlist();
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbPlaylistManager::SetPlaylistFocusItem( uint32_t playlistIndex, uint32_t playlistItemIndex )
{
    playlist_manager::get()->playlist_set_focus_item( playlistIndex, playlistItemIndex );
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbPlaylistManager::SetPlaylistFocusItemByHandle( uint32_t playlistIndex, JsFbMetadbHandle* handle )
{
    if ( !handle )
    {
        JS_ReportErrorUTF8( pJsCtx_, "handle argument is null" );
        return std::nullopt;
    }

    playlist_manager::get()->playlist_set_focus_by_handle( playlistIndex, handle->GetHandle() );
    return nullptr;
}

std::optional<std::nullptr_t> 
JsFbPlaylistManager::SetPlaylistSelection( uint32_t playlistIndex, JS::HandleValue affectedItems, bool state )
{
    JS::RootedObject jsObject( pJsCtx_, affectedItems.toObjectOrNull() );
    if ( !jsObject )
    {
        JS_ReportErrorUTF8( pJsCtx_, "affectedItems argument is not a JS object" );
        return std::nullopt;
    }

    bool is;
    if ( !JS_IsArrayObject( pJsCtx_, jsObject, &is ) )
    {
        JS_ReportErrorUTF8( pJsCtx_, "affectedItems argument is an array" );
        return std::nullopt;
    }

    uint32_t arraySize;
    if ( !JS_GetArrayLength( pJsCtx_, jsObject, &arraySize ) )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Failed to get affectedItems argument array length" );
        return std::nullopt;
    }

    auto api = playlist_manager::get();
    pfc::bit_array_bittable affected( api->playlist_get_item_count( playlistIndex ) );

    JS::RootedValue arrayElement( pJsCtx_ );
    for ( uint32_t i = 0; i < arraySize; ++i )
    {
        if ( !JS_GetElement( pJsCtx_, jsObject, i, &arrayElement ) )
        {
            JS_ReportErrorUTF8( pJsCtx_, "Failed to get affectedItems[%u]", i );
            return std::nullopt;
        }

        bool isValid;
        uint32_t affectedIdx( convert::to_native::ToValue<uint32_t>( pJsCtx_, arrayElement, isValid ) );
        if ( !isValid )
        {
            JS_ReportErrorUTF8( pJsCtx_, "affectedItems[%u] can't be converted to number" );
            return std::nullopt;
        }

        affected.set( affectedIdx, true );
    }

    pfc::bit_array_val status( state );
    api->playlist_set_selection( playlistIndex, affected, status );

    return nullptr;
}

std::optional<std::nullptr_t>
JsFbPlaylistManager::SetPlaylistSelectionSingle( uint32_t playlistIndex, uint32_t playlistItemIndex, bool state )
{
    playlist_manager::get()->playlist_set_selection_single( playlistIndex, playlistItemIndex, state );
    return nullptr;
}

std::optional<bool>
JsFbPlaylistManager::ShowAutoPlaylistUI( uint32_t playlistIndex )
{
    if ( playlistIndex >= playlist_manager::get()->get_playlist_count() )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Index is out of bounds" );
        return std::nullopt;
    }

    auto api = autoplaylist_manager::get();
    if ( !api->is_client_present( playlistIndex ) )
    {
        return false;
    }

    autoplaylist_client_ptr client = api->query_client( playlistIndex );
    client->show_ui( playlistIndex );

    return true;
}

std::optional<bool>
JsFbPlaylistManager::SortByFormat( uint32_t playlistIndex, const pfc::string8_fast& pattern, bool selOnly )
{
    return playlist_manager::get()->playlist_sort_by_format( playlistIndex, pattern.is_empty() ? nullptr : pattern.c_str(), selOnly );
}

std::optional<bool> 
JsFbPlaylistManager::SortByFormatWithOpt( size_t optArgCount, uint32_t playlistIndex, const pfc::string8_fast& pattern, bool selOnly )
{
    if ( optArgCount > 1 )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: invalid number of optional arguments specified: %d", optArgCount );
        return std::nullopt;
    }

    if ( optArgCount == 1 )
    {
        return SortByFormat( playlistIndex, pattern );
    }

    return SortByFormat( playlistIndex, pattern, selOnly );
}

std::optional<bool>
JsFbPlaylistManager::SortByFormatV2( uint32_t playlistIndex, const pfc::string8_fast& pattern, int8_t direction )
{
    auto api = playlist_manager::get();
    metadb_handle_list handles;
    api->playlist_get_all_items( playlistIndex, handles );

    pfc::array_t<t_size> order;
    order.set_count( handles.get_count() );

    titleformat_object::ptr script;
    titleformat_compiler::get()->compile_safe( script, pattern.c_str() );

    metadb_handle_list_helper::sort_by_format_get_order( handles, order.get_ptr(), script, nullptr, direction );

    return api->playlist_reorder_items( playlistIndex, order.get_ptr(), order.get_count() );
}

std::optional<bool> 
JsFbPlaylistManager::SortByFormatV2WithOpt( size_t optArgCount, uint32_t playlistIndex, const pfc::string8_fast& pattern, int8_t direction )
{
    if ( optArgCount > 1 )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: invalid number of optional arguments specified: %d", optArgCount );
        return std::nullopt;
    }

    if ( optArgCount == 1 )
    {
        return SortByFormatV2( playlistIndex, pattern );
    }

    return SortByFormatV2( playlistIndex, pattern, direction );
}

std::optional<std::nullptr_t>
JsFbPlaylistManager::SortPlaylistsByName( int8_t direction )
{
    // TODO: investigate this code
    auto api = playlist_manager::get();
    t_size i, count = api->get_playlist_count();

    pfc::array_t<helpers::custom_sort_data> data;
    data.set_size( count );

    pfc::string8_fastalloc temp;
    temp.prealloc( 512 );

    for ( i = 0; i < count; ++i )
    {
        api->playlist_get_name( i, temp );
        data[i].index = i;
        data[i].text = helpers::make_sort_string( temp );
    }

    pfc::sort_t( data, direction > 0 ? helpers::custom_sort_compare<1> : helpers::custom_sort_compare<-1>, count );
    order_helper order( count );

    for ( i = 0; i < count; ++i )
    {
        order[i] = data[i].index;
        delete[] data[i].text;
    }

    api->reorder( order.get_ptr(), order.get_count() );
    return nullptr;
}

std::optional<std::nullptr_t> 
JsFbPlaylistManager::SortPlaylistsByNameWithOpt( size_t optArgCount, int8_t direction )
{
    if ( optArgCount > 1 )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: invalid number of optional arguments specified: %d", optArgCount );
        return std::nullopt;
    }

    if ( optArgCount == 1 )
    {
        return SortPlaylistsByName();
    }

    return SortPlaylistsByName( direction );
}

std::optional<std::nullptr_t>
JsFbPlaylistManager::UndoBackup( uint32_t playlistIndex )
{
    playlist_manager::get()->playlist_undo_backup( playlistIndex );
    return nullptr;
}

std::optional<int32_t>
JsFbPlaylistManager::get_ActivePlaylist()
{
    uint32_t upos = playlist_manager::get()->get_active_playlist();
    return (pfc_infinite == upos ? -1 : static_cast<int32_t>(upos));
}

std::optional<uint32_t>
JsFbPlaylistManager::get_PlaybackOrder()
{
    return playlist_manager::get()->playback_order_get_active();
}

std::optional<int32_t>
JsFbPlaylistManager::get_PlayingPlaylist()
{
    uint32_t upos = playlist_manager::get()->get_playing_playlist();
    return (pfc_infinite == upos ? -1 : static_cast<int32_t>(upos));
}

std::optional<uint32_t>
JsFbPlaylistManager::get_PlaylistCount()
{
    return playlist_manager::get()->get_playlist_count();
}

std::optional<JSObject*>
JsFbPlaylistManager::get_PlaylistRecyclerManager()
{
    if ( !jsPlaylistRecycler_.initialized() )
    {
        jsPlaylistRecycler_.init( pJsCtx_, JsFbPlaylistRecyclerManager::Create( pJsCtx_ ) );
        if ( !jsPlaylistRecycler_ )
        {
            JS_ReportErrorUTF8( pJsCtx_, "Internal error: failed to create JS object" );
            return std::nullopt;
        }
    }

    return jsPlaylistRecycler_;
}

std::optional<std::nullptr_t>
JsFbPlaylistManager::put_ActivePlaylist( int32_t playlistIndex )
{
    t_size index = playlistIndex >= 0 ? static_cast<t_size>(playlistIndex) : pfc_infinite;
    playlist_manager::get()->set_active_playlist( index );
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbPlaylistManager::put_PlaybackOrder( uint32_t order )
{
    playlist_manager::get()->playback_order_set_active( order );
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbPlaylistManager::put_PlayingPlaylist( int32_t playlistIndex )
{
    t_size index = playlistIndex >= 0 ? static_cast<t_size>(playlistIndex) : pfc_infinite;
    playlist_manager::get()->set_playing_playlist( index );
    return nullptr;
}

}
