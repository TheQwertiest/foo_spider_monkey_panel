#pragma once

#include <optional>
#include <string>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsFbMetadbHandle;
class JsFbMetadbHandleList;
class JsFbPlayingItemLocation;
class JsFbPlaylistRecyclerManager;

class JsFbPlaylistManager
{
public:
    ~JsFbPlaylistManager();

    static JSObject* Create( JSContext* cx );

    static const JSClass& GetClass();

public:
    std::optional<std::nullptr_t> AddItemToPlaybackQueue( JsFbMetadbHandle* handle );
    //std::optional<std::nullptr_t> AddLocations( uint32_t playlistIndex, VARIANT locations, bool select );
    std::optional<std::nullptr_t> AddPlaylistItemToPlaybackQueue( uint32_t playlistIndex, uint32_t playlistItemIndex );
    std::optional<std::nullptr_t> ClearPlaylist( uint32_t playlistIndex );
    std::optional<std::nullptr_t> ClearPlaylistSelection( uint32_t playlistIndex );
    std::optional<int32_t> CreateAutoPlaylist( uint32_t playlistIndex, const std::string& name, const std::string&  query, const std::string&  sort, uint32_t flags );
    std::optional<int32_t> CreatePlaylist( uint32_t playlistIndex, const std::string& name );
    std::optional<uint32_t> DuplicatePlaylist( uint32_t from, const std::string&  name );
    std::optional<std::nullptr_t> EnsurePlaylistItemVisible( uint32_t playlistIndex, uint32_t playlistItemIndex );
    std::optional<bool> ExecutePlaylistDefaultAction( uint32_t playlistIndex, uint32_t playlistItemIndex );
    std::optional<int32_t> FindOrCreatePlaylist( const std::string& name, bool unlocked );
    std::optional<int32_t> FindPlaybackQueueItemIndex( JsFbMetadbHandle* handle, uint32_t playlistIndex, uint32_t playlistItemIndex );
    std::optional<int32_t> FindPlaylist( const std::string& name );
    std::optional<std::nullptr_t> FlushPlaybackQueue();
    //std::optional<std::nullptr_t> GetPlaybackQueueContents( VARIANT* outContents );
    std::optional<JSObject*> GetPlaybackQueueHandles();
    std::optional<JSObject*> GetPlayingItemLocation();
    std::optional<int32_t> GetPlaylistFocusItemIndex( uint32_t playlistIndex );
    std::optional<JSObject*> GetPlaylistItems( uint32_t playlistIndex );
    std::optional<std::string> GetPlaylistName( uint32_t playlistIndex );
    std::optional<JSObject*> GetPlaylistSelectedItems( uint32_t playlistIndex );
    std::optional<std::nullptr_t> InsertPlaylistItems( uint32_t playlistIndex, uint32_t base, JsFbMetadbHandleList* handles, bool select );
    std::optional<std::nullptr_t> InsertPlaylistItemsFilter( uint32_t playlistIndex, uint32_t base, JsFbMetadbHandleList* handles, bool select );
    std::optional<bool> IsAutoPlaylist( uint32_t playlistIndex );
    std::optional<bool> IsPlaylistItemSelected( uint32_t playlistIndex, uint32_t playlistItemIndex );
    std::optional<bool> IsPlaylistLocked( uint32_t playlistIndex );
    std::optional<bool> MovePlaylist( uint32_t from, uint32_t to );
    std::optional<bool> MovePlaylistSelection( uint32_t playlistIndex, int32_t delta );
    std::optional<uint32_t> PlaylistItemCount( uint32_t playlistIndex );
    std::optional<std::nullptr_t> RemoveItemFromPlaybackQueue( uint32_t index );
    //std::optional<std::nullptr_t> RemoveItemsFromPlaybackQueue( VARIANT affectedItems );
    std::optional<bool> RemovePlaylist( uint32_t playlistIndex );
    std::optional<std::nullptr_t> RemovePlaylistSelection( uint32_t playlistIndex, bool crop );
    std::optional<bool> RemovePlaylistSwitch( uint32_t playlistIndex );
    std::optional<bool> RenamePlaylist( uint32_t playlistIndex, const std::string& name );
    std::optional<std::nullptr_t> SetActivePlaylistContext();
    std::optional<std::nullptr_t> SetPlaylistFocusItem( uint32_t playlistIndex, uint32_t playlistItemIndex );
    std::optional<std::nullptr_t> SetPlaylistFocusItemByHandle( uint32_t playlistIndex, JsFbMetadbHandle* handle );
    //std::optional<std::nullptr_t> SetPlaylistSelection( uint32_t playlistIndex, VARIANT affectedItems, bool state );
    std::optional<std::nullptr_t> SetPlaylistSelectionSingle( uint32_t playlistIndex, uint32_t playlistItemIndex, bool state );
    std::optional<bool> ShowAutoPlaylistUI( uint32_t playlistIndex );
    std::optional<bool> SortByFormat( uint32_t playlistIndex, const std::string& pattern, bool selOnly );
    std::optional<bool> SortByFormatV2( uint32_t playlistIndex, const std::string& pattern, int8_t direction );
    std::optional<std::nullptr_t> SortPlaylistsByName( int8_t direction );
    std::optional<std::nullptr_t> UndoBackup( uint32_t playlistIndex );

public:
    std::optional<int32_t> get_ActivePlaylist();
    std::optional<uint32_t> get_PlaybackOrder();
    std::optional<int32_t> get_PlayingPlaylist();
    std::optional<uint32_t> get_PlaylistCount();
    std::optional<JSObject*> get_PlaylistRecyclerManager();
    std::optional<std::nullptr_t> put_ActivePlaylist( int32_t playlistIndex );
    std::optional<std::nullptr_t> put_PlaybackOrder( uint32_t order );
    std::optional<std::nullptr_t> put_PlayingPlaylist( int32_t playlistIndex );

private:
    JsFbPlaylistManager( JSContext* cx );
    JsFbPlaylistManager( const JsFbPlaylistManager& ) = delete;
    JsFbPlaylistManager& operator=( const JsFbPlaylistManager& ) = delete;

private:
    JSContext * pJsCtx_ = nullptr;
    // TODO: add to Create?
    JS::PersistentRootedObject jsPlaylistRecycler_;    
};

}
