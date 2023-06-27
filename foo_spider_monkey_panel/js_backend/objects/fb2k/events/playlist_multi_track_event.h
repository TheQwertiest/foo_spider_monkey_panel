#pragma once

#include <js_backend/objects/dom/event.h>
#include <js_backend/objects/fb2k/events/playlist_event.h>

namespace mozjs
{

class PlaylistMultiTrackEvent;

template <>
struct JsObjectTraits<PlaylistMultiTrackEvent>
{
    using ParentJsType = PlaylistEvent;

    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = true;
    static constexpr bool HasParentProto = true;
    static constexpr bool IsExtendable = true;

    static const JSClass JsClass;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
    static const JSNative JsConstructor;
};

class PlaylistMultiTrackEvent
    : public JsObjectBase<PlaylistMultiTrackEvent>
    , protected PlaylistEvent
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( PlaylistMultiTrackEvent );

    using ParentJsType = PlaylistEvent;

public:
    struct EventProperties
    {
        ParentJsType::EventProperties baseProps;
        std::vector<uint32_t> trackIndices;
    };

protected:
    struct EventOptions
    {
        ParentJsType::EventOptions baseOptions;
        std::vector<uint32_t> trackIndices;

        EventProperties ToDefaultProps() const;
    };

public:
    ~PlaylistMultiTrackEvent() override = default;

public:
    static JSObject* Constructor( JSContext* cx, const qwr::u8string& type, JS::HandleValue options = JS::UndefinedHandleValue );
    static JSObject* ConstructorWithOpt( JSContext* cx, size_t optArgCount, const qwr::u8string& type, JS::HandleValue options );

    const std::vector<uint32_t>& get_TrackIndices() const;

protected:
    [[nodiscard]] PlaylistMultiTrackEvent( JSContext* cx, const qwr::u8string& type, const EventProperties& props );
    [[nodiscard]] PlaylistMultiTrackEvent( JSContext* cx, const qwr::u8string& type, const EventOptions& options = {} );
    [[nodiscard]] size_t GetInternalSize() const;

    [[nodiscard]] static EventOptions ExtractOptions( JSContext* cx, JS::HandleValue options );

private:
    static std::unique_ptr<PlaylistMultiTrackEvent> CreateNative( JSContext* cx, const qwr::u8string& type, const EventProperties& props );
    static std::unique_ptr<PlaylistMultiTrackEvent> CreateNative( JSContext* cx, const qwr::u8string& type, const EventOptions& options );

private:
    JSContext* pJsCtx_ = nullptr;

    EventProperties props_;
};

} // namespace mozjs
