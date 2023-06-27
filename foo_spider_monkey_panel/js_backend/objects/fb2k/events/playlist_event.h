#pragma once

#include <js_backend/objects/dom/event.h>

namespace mozjs
{

class PlaylistEvent;

template <>
struct JsObjectTraits<PlaylistEvent>
{
    using ParentJsType = JsEvent;

    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = true;
    static constexpr bool HasParentProto = true;
    static constexpr bool IsExtendable = true;

    static const JSClass JsClass;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
    static const JSNative JsConstructor;
};

class PlaylistEvent
    : public JsObjectBase<PlaylistEvent>
    , protected JsEvent
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( PlaylistEvent );

    using ParentJsType = JsEvent;

public:
    struct EventProperties
    {
        ParentJsType::EventProperties baseProps;
        int32_t playlistIndex = -1;
    };

protected:
    struct EventOptions
    {
        ParentJsType::EventOptions baseOptions;
        int32_t playlistIndex = -1;

        EventProperties ToDefaultProps() const;
    };

public:
    ~PlaylistEvent() override = default;

public:
    static JSObject* Constructor( JSContext* cx, const qwr::u8string& type, JS::HandleValue options = JS::UndefinedHandleValue );
    static JSObject* ConstructorWithOpt( JSContext* cx, size_t optArgCount, const qwr::u8string& type, JS::HandleValue options );

    uint32_t get_PlaylistIndex() const;

protected:
    [[nodiscard]] PlaylistEvent( JSContext* cx, const qwr::u8string& type, const EventProperties& props );
    [[nodiscard]] PlaylistEvent( JSContext* cx, const qwr::u8string& type, const EventOptions& options = {} );
    [[nodiscard]] size_t GetInternalSize() const;

    [[nodiscard]] static EventOptions ExtractOptions( JSContext* cx, JS::HandleValue options );

private:
    static std::unique_ptr<PlaylistEvent> CreateNative( JSContext* cx, const qwr::u8string& type, const EventProperties& props );
    static std::unique_ptr<PlaylistEvent> CreateNative( JSContext* cx, const qwr::u8string& type, const EventOptions& options );

private:
    JSContext* pJsCtx_ = nullptr;

    EventProperties props_;
};

} // namespace mozjs
