#pragma once

#include <js_backend/objects/dom/event.h>

namespace mozjs
{

class LibraryTrackEvent;

template <>
struct JsObjectTraits<LibraryTrackEvent>
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

class LibraryTrackEvent
    : public JsObjectBase<LibraryTrackEvent>
    , protected JsEvent
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( LibraryTrackEvent );

    using ParentJsType = JsEvent;

public:
    struct EventProperties
    {
        ParentJsType::EventProperties baseProps;
        metadb_handle_list handles;
    };

protected:
    struct EventOptions
    {
        ParentJsType::EventOptions baseOptions;
        metadb_handle_list handles;

        EventProperties ToDefaultProps() const;
    };

public:
    ~LibraryTrackEvent() override = default;

public:
    static JSObject* Constructor( JSContext* cx, const qwr::u8string& type, JS::HandleValue options = JS::UndefinedHandleValue );
    static JSObject* ConstructorWithOpt( JSContext* cx, size_t optArgCount, const qwr::u8string& type, JS::HandleValue options );

    JSObject* get_Tracks() const;

protected:
    [[nodiscard]] LibraryTrackEvent( JSContext* cx, const qwr::u8string& type, const EventProperties& props );
    [[nodiscard]] LibraryTrackEvent( JSContext* cx, const qwr::u8string& type, const EventOptions& options = {} );
    [[nodiscard]] size_t GetInternalSize() const;

    [[nodiscard]] static EventOptions ExtractOptions( JSContext* cx, JS::HandleValue options );

private:
    static std::unique_ptr<LibraryTrackEvent> CreateNative( JSContext* cx, const qwr::u8string& type, const EventProperties& props );
    static std::unique_ptr<LibraryTrackEvent> CreateNative( JSContext* cx, const qwr::u8string& type, const EventOptions& options );

private:
    JSContext* pJsCtx_ = nullptr;

    EventProperties props_;
};

} // namespace mozjs
