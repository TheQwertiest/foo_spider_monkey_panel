#pragma once

#include <js_backend/objects/dom/mouse_event.h>

namespace mozjs
{

class WheelEvent;

template <>
struct JsObjectTraits<WheelEvent>
{
    using ParentJsType = MouseEvent;

    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = true;
    static constexpr bool HasParentProto = true;

    static const JSClass JsClass;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
    static const JSNative JsConstructor;
};

class WheelEvent
    : public JsObjectBase<WheelEvent>
    , protected MouseEvent
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( WheelEvent );

    using ParentJsType = MouseEvent;

public:
    struct EventProperties
    {
        ParentJsType::EventProperties baseProps;
        double deltaX = 0;
        double deltaY = 0;
        double deltaZ = 0;
        int32_t deltaMode = 0;
    };

protected:
    struct EventOptions
    {
        ParentJsType::EventOptions baseOptions;
        double deltaX = 0;
        double deltaY = 0;
        double deltaZ = 0;
        int32_t deltaMode = 0;

        EventProperties ToDefaultProps() const;
    };

public:
    ~WheelEvent() override = default;

public:
    static JSObject* Constructor( JSContext* cx, const qwr::u8string& type, JS::HandleValue options = JS::UndefinedHandleValue );
    static JSObject* ConstructorWithOpt( JSContext* cx, size_t optArgCount, const qwr::u8string& type, JS::HandleValue options );

    double get_DeltaX() const;
    double get_DeltaY() const;
    double get_DeltaZ() const;
    int32_t get_DeltaMode() const;

protected:
    [[nodiscard]] WheelEvent( JSContext* cx, const qwr::u8string& type, const EventProperties& props );
    [[nodiscard]] WheelEvent( JSContext* cx, const qwr::u8string& type, const EventOptions& options = {} );
    [[nodiscard]] size_t GetInternalSize();

    static EventOptions ExtractOptions( JSContext* cx, JS::HandleValue options );

private:
    static std::unique_ptr<WheelEvent> CreateNative( JSContext* cx, const qwr::u8string& type, const EventProperties& props );
    static std::unique_ptr<WheelEvent> CreateNative( JSContext* cx, const qwr::u8string& type, const EventOptions& options );

private:
    JSContext* pJsCtx_ = nullptr;

    EventProperties props_;
};

} // namespace mozjs
