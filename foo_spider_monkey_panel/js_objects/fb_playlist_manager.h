#pragma once

#include <js_objects/object_base.h>

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
class JsFbPlaylistRecycler;

class JsFbPlaylistManager
    : public JsObjectBase<JsFbPlaylistManager>
{
public:
    static constexpr bool HasProto = false;
    static constexpr bool HasProxy = false;
    static constexpr bool HasPostCreate = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;

public:
    ~JsFbPlaylistManager() override;

    static std::unique_ptr<JsFbPlaylistManager> CreateNative( JSContext* cx );
    static size_t GetInternalSize();

    void PrepareForGc();

public:
    void AddItemToPlaybackQueue( JsFbMetadbHandle* handle );
    void AddLocations( uint32_t playlistIndex, JS::HandleValue locations, bool select = false );
    void AddLocationsWithOpt( size_t optArgCount, uint32_t playlistIndex, JS::HandleValue locations, bool select );
    void AddPlaylistItemToPlaybackQueue( uint32_t playlistIndex, uint32_t playlistItemIndex );
    void ClearPlaylist( uint32_t playlistIndex );
    void ClearPlaylistSelection( uint32_t playlistIndex );
    uint32_t CreateAutoPlaylist( uint32_t playlistIndex, const qwr::u8string& name, const qwr::u8string& query, const qwr::u8string& sort = "", uint32_t flags = 0 );
    uint32_t CreateAutoPlaylistWithOpt( size_t optArgCount, uint32_t playlistIndex, const qwr::u8string& name, const qwr::u8string& query, const qwr::u8string& sort, uint32_t flags );
    uint32_t CreatePlaylist( uint32_t playlistIndex, const qwr::u8string& name );
    uint32_t DuplicatePlaylist( uint32_t from, const qwr::u8string& name = "" );
    uint32_t DuplicatePlaylistWithOpt( size_t optArgCount, uint32_t from, const qwr::u8string& name );
    void EnsurePlaylistItemVisible( uint32_t playlistIndex, uint32_t playlistItemIndex );
    bool ExecutePlaylistDefaultAction( uint32_t playlistIndex, uint32_t playlistItemIndex );
    uint32_t FindOrCreatePlaylist( const qwr::u8string& name, bool unlocked );
    int32_t FindPlaybackQueueItemIndex( JsFbMetadbHandle* handle, uint32_t playlistIndex, uint32_t playlistItemIndex );
    int32_t FindPlaylist( const qwr::u8string& name );
    void FlushPlaybackQueue();
    JS::Value GetPlaybackQueueContents();
    JSObject* GetPlaybackQueueHandles();
    JSObject* GetPlayingItemLocation();
    int32_t GetPlaylistFocusItemIndex( uint32_t playlistIndex );
    JSObject* GetPlaylistItems( uint32_t playlistIndex );
    std::optional<pfc::string8_fast> GetPlaylistLockName( uint32_t playlistIndex );
    JS::Value GetPlaylistLockedActions( uint32_t playlistIndex );
    pfc::string8_fast GetPlaylistName( uint32_t playlistIndex );
    JSObject* GetPlaylistSelectedItems( uint32_t playlistIndex );
    void InsertPlaylistItems( uint32_t playlistIndex, uint32_t base, JsFbMetadbHandleList* handles, bool select = false );
    void InsertPlaylistItemsWithOpt( size_t optArgCount, uint32_t playlistIndex, uint32_t base, JsFbMetadbHandleList* handles, bool select );
    void InsertPlaylistItemsFilter( uint32_t playlistIndex, uint32_t base, JsFbMetadbHandleList* handles, bool select = false );
    void InsertPlaylistItemsFilterWithOpt( size_t optArgCount, uint32_t playlistIndex, uint32_t base, JsFbMetadbHandleList* handles, bool select );
    bool IsAutoPlaylist( uint32_t playlistIndex );
    bool IsPlaylistItemSelected( uint32_t playlistIndex, uint32_t playlistItemIndex );
    // TODO v2: remove
    bool IsPlaylistLocked( uint32_t playlistIndex );
    bool IsRedoAvailable( uint32_t playlistIndex );
    bool IsUndoAvailable( uint32_t playlistIndex );
    bool MovePlaylist( uint32_t from, uint32_t to );
    bool MovePlaylistSelection( uint32_t playlistIndex, int32_t delta );
    uint32_t PlaylistItemCount( uint32_t playlistIndex );
    void Redo( uint32_t playlistIndex );
    void RemoveItemFromPlaybackQueue( uint32_t index );
    void RemoveItemsFromPlaybackQueue( JS::HandleValue affectedItems );
    bool RemovePlaylist( uint32_t playlistIndex );
    void RemovePlaylistSelection( uint32_t playlistIndex, bool crop = false );
    void RemovePlaylistSelectionWithOpt( size_t optArgCount, uint32_t playlistIndex, bool crop );
    bool RemovePlaylistSwitch( uint32_t playlistIndex );
    bool RenamePlaylist( uint32_t playlistIndex, const qwr::u8string& name );
    void SetActivePlaylistContext();
    void SetPlaylistFocusItem( uint32_t playlistIndex, uint32_t playlistItemIndex );
    void SetPlaylistFocusItemByHandle( uint32_t playlistIndex, JsFbMetadbHandle* handle );
    void SetPlaylistLockedActions( uint32_t playlistIndex, JS::HandleValue lockedActions = JS::NullHandleValue );
    void SetPlaylistLockedActionsWithOpt( size_t optArgCount, uint32_t playlistIndex, JS::HandleValue lockedActions );
    void SetPlaylistSelection( uint32_t playlistIndex, JS::HandleValue affectedItems, bool state );
    void SetPlaylistSelectionSingle( uint32_t playlistIndex, uint32_t playlistItemIndex, bool state );
    bool ShowAutoPlaylistUI( uint32_t playlistIndex );
    bool SortByFormat( uint32_t playlistIndex, const qwr::u8string& pattern, bool selOnly = false );
    bool SortByFormatWithOpt( size_t optArgCount, uint32_t playlistIndex, const qwr::u8string& pattern, bool selOnly );
    bool SortByFormatV2( uint32_t playlistIndex, const qwr::u8string& pattern, int8_t direction = 1 );
    bool SortByFormatV2WithOpt( size_t optArgCount, uint32_t playlistIndex, const qwr::u8string& pattern, int8_t direction );
    void SortPlaylistsByName( int8_t direction = 1 );
    void SortPlaylistsByNameWithOpt( size_t optArgCount, int8_t direction );
    void Undo( uint32_t playlistIndex );
    void UndoBackup( uint32_t playlistIndex );

public:
    int32_t get_ActivePlaylist();
    uint32_t get_PlaybackOrder();
    int32_t get_PlayingPlaylist();
    uint32_t get_PlaylistCount();
    JSObject* get_PlaylistRecycler();
    void put_ActivePlaylist( uint32_t playlistIndex );
    void put_PlaybackOrder( uint32_t order );
    void put_PlayingPlaylist( uint32_t playlistIndex );

private:
    JsFbPlaylistManager( JSContext* cx );

private:
    JSContext* pJsCtx_ = nullptr;
    JS::PersistentRootedObject jsPlaylistRecycler_;
};

} // namespace mozjs
