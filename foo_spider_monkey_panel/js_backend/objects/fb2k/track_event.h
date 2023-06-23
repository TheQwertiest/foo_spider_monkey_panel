#pragma once

#include <js_backend/objects/dom/event.h>

namespace mozjs
{

class TrackEvent;

template <>
struct JsObjectTraits<TrackEvent>
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

class TrackEvent
    : public JsObjectBase<TrackEvent>
    , protected JsEvent
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( TrackEvent );

    using ParentJsType = JsEvent;

public:
    struct EventProperties
    {
        ParentJsType::EventProperties baseProps;
        metadb_handle_list affectedTracks;
    };

protected:
    struct EventOptions
    {
        ParentJsType::EventOptions baseOptions;
        metadb_handle_list affectedTracks;

        EventProperties ToDefaultProps() const;
    };

public:
    ~TrackEvent() override = default;

public:
    static JSObject* Constructor( JSContext* cx, const qwr::u8string& type, JS::HandleValue options = JS::UndefinedHandleValue );
    static JSObject* ConstructorWithOpt( JSContext* cx, size_t optArgCount, const qwr::u8string& type, JS::HandleValue options );

    JSObject* get_AffectedTracks() const;

protected:
    [[nodiscard]] TrackEvent( JSContext* cx, const qwr::u8string& type, const EventProperties& props );
    [[nodiscard]] TrackEvent( JSContext* cx, const qwr::u8string& type, const EventOptions& options = {} );
    [[nodiscard]] size_t GetInternalSize() const;

    [[nodiscard]] static EventOptions ExtractOptions( JSContext* cx, JS::HandleValue options );

private:
    static std::unique_ptr<TrackEvent> CreateNative( JSContext* cx, const qwr::u8string& type, const EventProperties& props );
    static std::unique_ptr<TrackEvent> CreateNative( JSContext* cx, const qwr::u8string& type, const EventOptions& options );

private:
    JSContext* pJsCtx_ = nullptr;

    EventProperties props_;
};

} // namespace mozjs
