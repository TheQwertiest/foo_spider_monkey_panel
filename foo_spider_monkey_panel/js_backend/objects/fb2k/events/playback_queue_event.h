#pragma once

#include <js_backend/objects/dom/event.h>

namespace mozjs
{

class PlaybackQueueEvent;

template <>
struct JsObjectTraits<PlaybackQueueEvent>
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

class PlaybackQueueEvent
    : public JsObjectBase<PlaybackQueueEvent>
    , protected JsEvent
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( PlaybackQueueEvent );

    using ParentJsType = JsEvent;

public:
    struct EventProperties
    {
        ParentJsType::EventProperties baseProps;
        qwr::u8string origin = "unknown";
    };

protected:
    struct EventOptions
    {
        ParentJsType::EventOptions baseOptions;
        qwr::u8string origin = "unknown";

        EventProperties ToDefaultProps() const;
    };

public:
    ~PlaybackQueueEvent() override = default;

public:
    static JSObject* Constructor( JSContext* cx, const qwr::u8string& type, JS::HandleValue options = JS::UndefinedHandleValue );
    static JSObject* ConstructorWithOpt( JSContext* cx, size_t optArgCount, const qwr::u8string& type, JS::HandleValue options );

    qwr::u8string get_Origin() const;

protected:
    [[nodiscard]] PlaybackQueueEvent( JSContext* cx, const qwr::u8string& type, const EventProperties& props );
    [[nodiscard]] PlaybackQueueEvent( JSContext* cx, const qwr::u8string& type, const EventOptions& options = {} );
    [[nodiscard]] size_t GetInternalSize() const;

    [[nodiscard]] static EventOptions ExtractOptions( JSContext* cx, JS::HandleValue options );

private:
    static std::unique_ptr<PlaybackQueueEvent> CreateNative( JSContext* cx, const qwr::u8string& type, const EventProperties& props );
    static std::unique_ptr<PlaybackQueueEvent> CreateNative( JSContext* cx, const qwr::u8string& type, const EventOptions& options );

private:
    JSContext* pJsCtx_ = nullptr;

    EventProperties props_;
};

} // namespace mozjs
