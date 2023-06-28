#pragma once

#include <js_backend/objects/core/object_base.h>
#include <js_backend/objects/dom/event_target.h>
#include <tasks/events/event_id.h>

#include <js/TypeDecls.h>

namespace mozjs
{

class Track;
class TrackList;
class PlaylistManager;

template <>
struct JsObjectTraits<PlaylistManager>
{
    using ParentJsType = JsEventTarget;

    static constexpr bool HasProto = false;
    static constexpr bool HasParentProto = true;
    static constexpr bool HasPostCreate = true;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const PostJsCreateFn PostCreate;
};

class PlaylistManager final
    : public JsObjectBase<PlaylistManager>
    , private JsEventTarget
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( PlaylistManager );

public:
    static const std::unordered_set<smp::EventId> kHandledEvents;

public:
    ~PlaylistManager() override;

    [[nodiscard]] static std::unique_ptr<PlaylistManager> CreateNative( JSContext* cx );
    [[nodiscard]] size_t GetInternalSize() const;
    static void PostCreate( JSContext* cx, JS::HandleObject self );

    static void Trace( JSTracer* trc, JSObject* obj );

    EventStatus HandleEvent( JS::HandleObject self, const smp::EventBase& event ) override;

public:
    // TODO: add event handling
    JSObject* CreateAutoPlaylist( const qwr::u8string& query, const qwr::u8string& sortQuery = "", uint32_t playlistIndex = pfc_infinite, const qwr::u8string& name = "", bool enforceSort = false );
    JSObject* CreateAutoPlaylistWithOpt( size_t optArgCount, const qwr::u8string& query, const qwr::u8string& sortQuery, uint32_t playlistIndex, const qwr::u8string& name, bool enforceSort );
    JSObject* CreatePlaylist( uint32_t playlistIndex = pfc_infinite, const qwr::u8string& name = "" );
    JSObject* CreatePlaylistWithOpt( size_t optArgCount, uint32_t playlistIndex, const qwr::u8string& name );
    void DeletePlaylist( uint32_t playlistIndex, bool switchIfActive = true );
    void DeletePlaylistWithOpt( size_t optArgCount, uint32_t playlistIndex, bool switchIfActive );
    uint32_t DuplicatePlaylist( uint32_t playlistIndexFrom, uint32_t playlistIndexTo, const pfc::string8_fast& name = "" );
    uint32_t DuplicatePlaylistWithOpt( size_t optArgCount, uint32_t playlistIndexFrom, uint32_t playlistIndexTo, const pfc::string8_fast& name );
    JSObject* FindPlaylist( const qwr::u8string& name ) const;
    JSObject* GetActivePlaylist() const;
    JSObject* GetCurrentlyPlayingPlaylist() const;
    JSObject* GetCurrentlyPlayingTrackLocation() const;
    JSObject* GetPlaylist( uint32_t playlistIndex ) const;
    uint32_t GetPlaylistCount() const;
    void MovePlaylist( uint32_t playlistIndexFrom, uint32_t playlistIndexTo );
    void OrderPlaylistsByName( int8_t direction = 1 );
    void OrderPlaylistsByNameWithOpt( size_t optArgCount, int8_t direction );
    void SetActivePlaylistAsUiEditContext();

public:
    // TODO: impl
    // JSObject* get_PlaylistRecycler();

private:
    [[nodiscard]] PlaylistManager( JSContext* cx );

    [[nodiscard]] const std::string& EventIdToType( smp::EventId eventId );
    [[nodiscard]] JSObject* GenerateEvent( const smp::EventBase& event, const qwr::u8string& eventType );

    [[nodiscard]] uint32_t CreatePlaylistImpl( uint32_t playlistIndex, const qwr::u8string& name );

private:
    JSContext* pJsCtx_ = nullptr;
    mutable std::unordered_map<uint64_t, JS::Heap<JSObject*>> idToPlaylist_;
};

} // namespace mozjs
