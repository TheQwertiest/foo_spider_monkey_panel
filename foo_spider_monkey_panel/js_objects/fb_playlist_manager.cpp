#include <stdafx.h>
#include "fb_playlist_manager.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_metadb_handle.h>
#include <js_objects/fb_metadb_handle_list.h>
#include <js_objects/fb_playing_item_location.h>
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
// MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, AddLocations               );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, AddPlaylistItemToPlaybackQueue);
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, ClearPlaylist                 );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, ClearPlaylistSelection        );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, CreateAutoPlaylist           );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, CreatePlaylist                );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, DuplicatePlaylist            );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, EnsurePlaylistItemVisible    );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, ExecutePlaylistDefaultAction  );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, FindOrCreatePlaylist          );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, FindPlaybackQueueItemIndex    );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, FindPlaylist                  );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, FlushPlaybackQueue           );
// MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, GetPlaybackQueueContents   );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, GetPlaybackQueueHandles       );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, GetPlayingItemLocation        );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, GetPlaylistFocusItemIndex     );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, GetPlaylistItems              );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, GetPlaylistName               );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, GetPlaylistSelectedItems      );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, InsertPlaylistItems           );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, InsertPlaylistItemsFilter     );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, IsAutoPlaylist                );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, IsPlaylistItemSelected        );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, IsPlaylistLocked              );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, MovePlaylist                  );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, MovePlaylistSelection         );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, PlaylistItemCount             );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, RemoveItemFromPlaybackQueue  );
// MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, RemoveItemsFromPlaybackQueue);
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, RemovePlaylist                );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, RemovePlaylistSelection       );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, RemovePlaylistSwitch          );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, RenamePlaylist               );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, SetActivePlaylistContext      );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, SetPlaylistFocusItem          );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, SetPlaylistFocusItemByHandle  );
// MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, SetPlaylistSelection       );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, SetPlaylistSelectionSingle    );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, ShowAutoPlaylistUI            );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, SortByFormat                  );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, SortByFormatV2                );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, SortPlaylistsByName           );
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbPlaylistManager, UndoBackup                    );

// TODO: set arg count
const JSFunctionSpec jsFunctions[] = {
    JS_FN("AddItemToPlaybackQueue", AddItemToPlaybackQueue, 0, DefaultPropsFlags() ),
    // JS_FN("AddLocations", AddLocations, 0, DefaultPropsFlags() ),
    JS_FN("AddPlaylistItemToPlaybackQueue", AddPlaylistItemToPlaybackQueue, 0, DefaultPropsFlags() ),
    JS_FN("ClearPlaylist", ClearPlaylist, 0, DefaultPropsFlags() ),
    JS_FN("ClearPlaylistSelection", ClearPlaylistSelection, 0, DefaultPropsFlags() ),
    JS_FN("CreateAutoPlaylist", CreateAutoPlaylist, 0, DefaultPropsFlags() ),
    JS_FN("CreatePlaylist", CreatePlaylist, 0, DefaultPropsFlags() ),
    JS_FN("DuplicatePlaylist", DuplicatePlaylist, 0, DefaultPropsFlags() ),
    JS_FN("EnsurePlaylistItemVisible", EnsurePlaylistItemVisible, 0, DefaultPropsFlags() ),
    JS_FN("ExecutePlaylistDefaultAction", ExecutePlaylistDefaultAction, 0, DefaultPropsFlags() ),
    JS_FN("FindOrCreatePlaylist", FindOrCreatePlaylist, 0, DefaultPropsFlags() ),
    JS_FN("FindPlaybackQueueItemIndex", FindPlaybackQueueItemIndex, 0, DefaultPropsFlags() ),
    JS_FN("FindPlaylist", FindPlaylist, 0, DefaultPropsFlags() ),
    JS_FN("FlushPlaybackQueue", FlushPlaybackQueue, 0, DefaultPropsFlags() ),
    // JS_FN("GetPlaybackQueueContents", GetPlaybackQueueContents, 0, DefaultPropsFlags() ),
    JS_FN("GetPlaybackQueueHandles", GetPlaybackQueueHandles, 0, DefaultPropsFlags() ),
    JS_FN("GetPlayingItemLocation", GetPlayingItemLocation, 0, DefaultPropsFlags() ),
    JS_FN("GetPlaylistFocusItemIndex", GetPlaylistFocusItemIndex, 0, DefaultPropsFlags() ),
    JS_FN("GetPlaylistItems", GetPlaylistItems, 0, DefaultPropsFlags() ),
    JS_FN("GetPlaylistName", GetPlaylistName, 0, DefaultPropsFlags() ),
    JS_FN("GetPlaylistSelectedItems", GetPlaylistSelectedItems, 0, DefaultPropsFlags() ),
    JS_FN("InsertPlaylistItems", InsertPlaylistItems, 0, DefaultPropsFlags() ),
    JS_FN("InsertPlaylistItemsFilter", InsertPlaylistItemsFilter, 0, DefaultPropsFlags() ),
    JS_FN("IsAutoPlaylist", IsAutoPlaylist, 0, DefaultPropsFlags() ),
    JS_FN("IsPlaylistItemSelected", IsPlaylistItemSelected, 0, DefaultPropsFlags() ),
    JS_FN("IsPlaylistLocked", IsPlaylistLocked, 0, DefaultPropsFlags() ),
    JS_FN("MovePlaylist", MovePlaylist, 0, DefaultPropsFlags() ),
    JS_FN("MovePlaylistSelection", MovePlaylistSelection, 0, DefaultPropsFlags() ),
    JS_FN("PlaylistItemCount", PlaylistItemCount, 0, DefaultPropsFlags() ),
    JS_FN("RemoveItemFromPlaybackQueue", RemoveItemFromPlaybackQueue, 0, DefaultPropsFlags() ),
    // JS_FN("RemoveItemsFromPlaybackQueue", RemoveItemsFromPlaybackQueue, 0, DefaultPropsFlags() ),
    JS_FN("RemovePlaylist", RemovePlaylist, 0, DefaultPropsFlags() ),
    JS_FN("RemovePlaylistSelection", RemovePlaylistSelection, 0, DefaultPropsFlags() ),
    JS_FN("RemovePlaylistSwitch", RemovePlaylistSwitch, 0, DefaultPropsFlags() ),
    JS_FN("RenamePlaylist", RenamePlaylist, 0, DefaultPropsFlags() ),
    JS_FN("SetActivePlaylistContext", SetActivePlaylistContext, 0, DefaultPropsFlags() ),
    JS_FN("SetPlaylistFocusItem", SetPlaylistFocusItem, 0, DefaultPropsFlags() ),
    JS_FN("SetPlaylistFocusItemByHandle", SetPlaylistFocusItemByHandle, 0, DefaultPropsFlags() ),
    // JS_FN("SetPlaylistSelection", SetPlaylistSelection, 0, DefaultPropsFlags() ),
    JS_FN("SetPlaylistSelectionSingle", SetPlaylistSelectionSingle, 0, DefaultPropsFlags() ),
    JS_FN("ShowAutoPlaylistUI", ShowAutoPlaylistUI, 0, DefaultPropsFlags() ),
    JS_FN("SortByFormat", SortByFormat, 0, DefaultPropsFlags() ),
    JS_FN("SortByFormatV2", SortByFormatV2, 0, DefaultPropsFlags() ),
    JS_FN("SortPlaylistsByName", SortPlaylistsByName, 0, DefaultPropsFlags() ),
    JS_FN("UndoBackup", UndoBackup, 0, DefaultPropsFlags() ),
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

JsFbPlaylistManager::JsFbPlaylistManager( JSContext* cx )
    : pJsCtx_( cx )
{
}


JsFbPlaylistManager::~JsFbPlaylistManager()
{
    jsPlaylistRecycler_.reset();
}

JSObject* JsFbPlaylistManager::Create( JSContext* cx )
{
    JS::RootedObject jsObj( cx,
                            JS_NewObject( cx, &jsClass ) );
    if ( !jsObj )
    {
        return nullptr;
    }

    if ( !JS_DefineFunctions( cx, jsObj, jsFunctions )
         || !JS_DefineProperties( cx, jsObj, jsProperties ) )
    {
        return nullptr;
    }

    JS_SetPrivate( jsObj, new JsFbPlaylistManager( cx ) );

    return jsObj;
}

const JSClass& JsFbPlaylistManager::GetClass()
{
    return jsClass;
}

std::optional<std::nullptr_t>
JsFbPlaylistManager::AddItemToPlaybackQueue( JsFbMetadbHandle* handle )
{
    if ( !handle )
    {
        JS_ReportErrorASCII( pJsCtx_, "handle argument is null" );
        return std::nullopt;
    }

    playlist_manager::get()->queue_add_item( handle->GetHandle() );
    return nullptr;
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
JsFbPlaylistManager::CreateAutoPlaylist( uint32_t playlistIndex, const std::string& name, const std::string& query, const std::string& sort, uint32_t flags )
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
JsFbPlaylistManager::CreatePlaylist( uint32_t playlistIndex, const std::string& name )
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
    
    return (pfc_infinite == upos ? -1 : static_cast<int32_t>(upos));
}

std::optional<uint32_t>
JsFbPlaylistManager::DuplicatePlaylist( uint32_t from, const std::string& name )
{
    auto api = playlist_manager_v4::get();

    if ( from >= api->get_playlist_count() )
    {
        JS_ReportErrorASCII( pJsCtx_, "Index is out of bounds" );
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
JsFbPlaylistManager::FindOrCreatePlaylist( const std::string& name, bool unlocked )
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
        JS_ReportErrorASCII( pJsCtx_, "handle argument is null" );
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
JsFbPlaylistManager::FindPlaylist( const std::string& name )
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
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
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
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
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
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    return jsObject;
}

std::optional<std::string>
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
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    return jsObject;
}

std::optional<std::nullptr_t>
JsFbPlaylistManager::InsertPlaylistItems( uint32_t playlistIndex, uint32_t base, JsFbMetadbHandleList* handles, bool select )
{    
    if ( !handles )
    {
        JS_ReportErrorASCII( pJsCtx_, "handles argument is null" );
        return std::nullopt;
    }

    pfc::bit_array_val selection( select );
    playlist_manager::get()->playlist_insert_items( playlistIndex, base, handles->GetHandleList(), selection );
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbPlaylistManager::InsertPlaylistItemsFilter( uint32_t playlistIndex, uint32_t base, JsFbMetadbHandleList* handles, bool select )
{    
    if ( !handles )
    {
        JS_ReportErrorASCII( pJsCtx_, "handles argument is null" );
        return std::nullopt;
    }

    playlist_manager::get()->playlist_insert_items_filter( playlistIndex, base, handles->GetHandleList(), select );
    return nullptr;
}

std::optional<bool>
JsFbPlaylistManager::IsAutoPlaylist( uint32_t playlistIndex )
{
    if ( playlistIndex < 0 || playlistIndex >= playlist_manager::get()->get_playlist_count() )
    {
        JS_ReportErrorASCII( pJsCtx_, "Index is out of bounds" );
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
    if ( playlistIndex < 0 || playlistIndex >= api->get_playlist_count() )
    {
        JS_ReportErrorASCII( pJsCtx_, "Index is out of bounds" );
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

std::optional<bool>
JsFbPlaylistManager::RemovePlaylistSwitch( uint32_t playlistIndex )
{
    return playlist_manager::get()->remove_playlist_switch( playlistIndex );
}

std::optional<bool>
JsFbPlaylistManager::RenamePlaylist( uint32_t playlistIndex, const std::string& name )
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
        JS_ReportErrorASCII( pJsCtx_, "handle argument is null" );
        return std::nullopt;
    }

    playlist_manager::get()->playlist_set_focus_by_handle( playlistIndex, handle->GetHandle() );
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
    if ( playlistIndex < 0 || playlistIndex >= playlist_manager::get()->get_playlist_count() )
    {
        JS_ReportErrorASCII( pJsCtx_, "Index is out of bounds" );
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
JsFbPlaylistManager::SortByFormat( uint32_t playlistIndex, const std::string& pattern, bool selOnly )
{
    return playlist_manager::get()->playlist_sort_by_format( playlistIndex, pattern.empty() ? nullptr : pattern.c_str(), selOnly );
}

std::optional<bool>
JsFbPlaylistManager::SortByFormatV2( uint32_t playlistIndex, const std::string& pattern, int8_t direction )
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
            JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
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
