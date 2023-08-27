#pragma once

#include <js_backend/objects/core/object_base.h>
#include <js_backend/objects/dom/event_target.h>
#include <tasks/events/event_id.h>

#include <js/TypeDecls.h>

#include <unordered_set>

namespace mozjs
{

class Track;
class PlaybackQueue;

template <>
struct JsObjectTraits<PlaybackQueue>
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

class PlaybackQueue final
    : public JsObjectBase<PlaybackQueue>
    , private JsEventTarget
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( PlaybackQueue );

public:
    static const std::unordered_set<smp::EventId> kHandledEvents;

public:
    ~PlaybackQueue() override;

    [[nodiscard]] static std::unique_ptr<PlaybackQueue> CreateNative( JSContext* cx );
    [[nodiscard]] size_t GetInternalSize() const;
    static void PostCreate( JSContext* cx, JS::HandleObject self );

    static void Trace( JSTracer* trc, JSObject* obj );

    EventStatus HandleEvent( JS::HandleObject self, const smp::EventBase& event ) override;

public:
    void Clear();
    int32_t IndexOf( smp::not_null<const Track*> track, int32_t playlistIndex, int32_t trackIndex ) const;
    void PullAt( const std::vector<uint32_t>& itemIndices );
    void Push_Fake( JS::HandleValue arg1, JS::HandleValue arg2 );
    void Push_1( smp::not_null<const Track*> track );
    void Push_2( uint32_t playlistIndex, uint32_t trackIndex );
    void PushWithOpt( size_t optArgCount, JS::HandleValue arg1, JS::HandleValue arg2 );
    JS::Value ToArray() const;
    uint32_t get_Length() const;

private:
    [[nodiscard]] PlaybackQueue( JSContext* cx );

    [[nodiscard]] const std::string& EventIdToType( smp::EventId eventId ) const;

private:
    JSContext* pJsCtx_ = nullptr;
};

} // namespace mozjs
