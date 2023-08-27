#pragma once

#include <js_backend/objects/dom/event.h>

namespace mozjs
{

class MultiPlaylistEvent;

template <>
struct JsObjectTraits<MultiPlaylistEvent>
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

class MultiPlaylistEvent
    : public JsObjectBase<MultiPlaylistEvent>
    , protected JsEvent
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( MultiPlaylistEvent );

    using ParentJsType = JsEvent;

public:
    struct EventProperties
    {
        ParentJsType::EventProperties baseProps;
        std::vector<uint32_t> playlistIndices;
    };

protected:
    struct EventOptions
    {
        ParentJsType::EventOptions baseOptions;
        std::vector<uint32_t> playlistIndices;

        EventProperties ToDefaultProps() const;
    };

public:
    ~MultiPlaylistEvent() override = default;

public:
    static JSObject* Constructor( JSContext* cx, const qwr::u8string& type, JS::HandleValue options = JS::UndefinedHandleValue );
    static JSObject* ConstructorWithOpt( JSContext* cx, size_t optArgCount, const qwr::u8string& type, JS::HandleValue options );

    const std::vector<uint32_t>& get_PlaylistIndices() const;

protected:
    [[nodiscard]] MultiPlaylistEvent( JSContext* cx, const qwr::u8string& type, const EventProperties& props );
    [[nodiscard]] MultiPlaylistEvent( JSContext* cx, const qwr::u8string& type, const EventOptions& options = {} );
    [[nodiscard]] size_t GetInternalSize() const;

    [[nodiscard]] static EventOptions ExtractOptions( JSContext* cx, JS::HandleValue options );

private:
    static std::unique_ptr<MultiPlaylistEvent> CreateNative( JSContext* cx, const qwr::u8string& type, const EventProperties& props );
    static std::unique_ptr<MultiPlaylistEvent> CreateNative( JSContext* cx, const qwr::u8string& type, const EventOptions& options );

private:
    JSContext* pJsCtx_ = nullptr;

    EventProperties props_;
};

} // namespace mozjs
