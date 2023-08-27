#pragma once

#include <js_backend/objects/core/object_base.h>
#include <js_backend/objects/dom/event_target.h>
#include <tasks/events/event_id.h>

#include <js/TypeDecls.h>

namespace mozjs
{

class Track;
class TrackList;
class Library;

template <>
struct JsObjectTraits<Library>
{
    using ParentJsType = JsEventTarget;

    static constexpr bool HasProto = false;
    static constexpr bool HasParentProto = true;
    static constexpr bool HasPostCreate = true;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const PostJsCreateFn PostCreate;
};

class Library final
    : public JsObjectBase<Library>
    , private JsEventTarget
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( Library );

public:
    static const std::unordered_set<smp::EventId> kHandledEvents;

public:
    ~Library() override;

    [[nodiscard]] static std::unique_ptr<Library> CreateNative( JSContext* cx );
    [[nodiscard]] size_t GetInternalSize() const;
    static void PostCreate( JSContext* cx, JS::HandleObject self );

    static void Trace( JSTracer* trc, JSObject* obj );

    EventStatus HandleEvent( JS::HandleObject self, const smp::EventBase& event ) override;

public:
    // TODO: add AbortSignal to abortables
    bool Contains( smp::not_null<Track*> track ) const;
    JSObject* FilterTracks( JS::HandleValue tracks, const qwr::u8string& query = "" ) const;
    JSObject* FilterTracksWithOpt( size_t optArgCount, JS::HandleValue tracks, const qwr::u8string& query ) const;
    JSObject* GetTracks( const qwr::u8string& query = "" ) const;
    JSObject* GetTracksWithOpt( size_t optArgCount, const qwr::u8string& query ) const;
    std::vector<pfc::string8_fast> GetTracksRelativePath( JS::HandleValue tracks ) const;
    // TODO: add array support
    void OrderTracksByRelativePath( smp::not_null<TrackList*> tracks );
    // TODO: make query optional
    void ShowSearchUi( const qwr::u8string& query );

    bool get_IsEnabled() const;

private:
    [[nodiscard]] Library( JSContext* cx );

    [[nodiscard]] const std::string& EventIdToType( smp::EventId eventId );

private:
    JSContext* pJsCtx_ = nullptr;
};

} // namespace mozjs
