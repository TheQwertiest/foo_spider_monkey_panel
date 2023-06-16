#pragma once

#include <js_backend/objects/core/object_base.h>

#include <js/TypeDecls.h>

namespace mozjs
{

class Track;
class TrackList;
class Playlist;

template <>
struct JsObjectTraits<Playlist>
{
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = false;
    static constexpr bool HasProxy = true;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
    static const js::BaseProxyHandler& JsProxy;
};

class Playlist
    : public JsObjectBase<Playlist>
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( Playlist );

public:
    ~Playlist() override;

    [[nodiscard]] static std::unique_ptr<Playlist> CreateNative( JSContext* cx, uint32_t index );
    [[nodiscard]] size_t GetInternalSize() const;

    JS::Value GetItem( int32_t trackIndex ) const;
    void PutItem( int32_t trackIndex, smp::not_null<Track*> track );
    uint32_t GetItemCount() const;

public:
    // TODO: maybe add a promise?
    void AddPaths( const std::vector<qwr::u8string>& paths, uint32_t start = pfc_infinite, JS::HandleValue options = JS::UndefinedHandleValue );
    void AddPathsWithOpt( size_t optArgCount, const std::vector<qwr::u8string>& paths, uint32_t start, JS::HandleValue options );
    void ClearSelection();
    void CreateUndoPoint();
    void EnsureTrackVisible( uint32_t trackIndex );
    void ExecuteDefaultTrackAction( uint32_t trackIndex ) const;
    void FocusTrackByIndex( uint32_t trackIndex );
    void FocusTrackByValue( smp::not_null<Track*> track );
    pfc::string8_fast GetAutoPlaylistDisplayName() const;
    int32_t GetFocusedTrackIndex() const;
    std::optional<pfc::string8_fast> GetLockName() const;
    std::vector<qwr::u8string> GetLockedOperations() const;
    JSObject* GetSelectedTracks() const;
    std::vector<uint32_t> GetSelectedTrackIndices() const;
    JSObject* GetTracks() const;
    void InsertTracks( JS::HandleValue tracks, uint32_t start = pfc_infinite, JS::HandleValue options = JS::UndefinedHandleValue );
    void InsertTracksWithOpt( size_t optArgCount, JS::HandleValue tracks, uint32_t start, JS::HandleValue options );
    bool IsAutoPlaylistUiAvailable() const;
    bool IsLocked() const;
    bool IsRedoAvailable() const;
    bool IsTrackSelected( uint32_t trackIndex ) const;
    bool IsUndoAvailable() const;
    void MoveSelection( int32_t delta );
    void OrderTracksByFormat( const qwr::u8string& query, int8_t direction = 1, bool sortSelectedOnly = false );
    void OrderTracksByFormatWithOpt( size_t optArgCount, const qwr::u8string& query, int8_t direction, bool sortSelectedOnly );
    void Redo();
    void RemoveAll();
    void RemoveSelectedTracks();
    void RemoveUnselectedTracks();
    void SelectTracks( const std::vector<uint32_t>& trackIndices, bool invertSelection = false );
    void SelectTracksWithOpt( size_t optArgCount, const std::vector<uint32_t>& trackIndices, bool invertSelection );
    void SetAsActive();
    void SetAsCurrentlyPlaying();
    void SetLockedOperations( JS::HandleValue lockedOperations = JS::UndefinedHandleValue );
    void SetLockedOperationsWithOpt( size_t optArgCount, JS::HandleValue lockedOperations );
    void ShowAutoPlaylistUi() const;
    void Undo();

    JSObject* CreateIterator( JS::HandleObject jsSelf ) const;

    uint32_t get_Index() const;
    bool get_IsValid() const;
    bool get_IsAutoPlaylist() const;
    uint32_t get_Length() const;
    pfc::string8_fast get_Name() const;
    void put_Name( const qwr::u8string& value );

private:
    [[nodiscard]] Playlist( JSContext* cx, uint32_t index );

    uint32_t GetPlaylistIndex() const;

private:
    JSContext* pJsCtx_ = nullptr;
    const uint64_t id_;
};

} // namespace mozjs
