#pragma once

#include <js_backend/objects/core/object_traits.h>
#include <js_backend/objects/dom/event.h>

namespace mozjs
{

class MouseEvent;

template <>
struct JsObjectTraits<MouseEvent>
{
    using ParentJsType = JsEvent;

    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = true;
    static constexpr bool HasParentProto = true;
    static constexpr bool IsExtendable = true;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
    static const JSNative JsConstructor;
};

class MouseEvent
    : public JsObjectBase<MouseEvent>
    , protected JsEvent
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( MouseEvent );

    using ParentJsType = JsEvent;

public:
    struct EventProperties
    {
        ParentJsType::EventProperties baseProps;
        bool altKey = false;
        bool ctrlKey = false;
        bool metaKey = false;
        bool shiftKey = false;
        int32_t button = 0;
        int32_t buttons = 0;
        double screenX = 0;
        double screenY = 0;
        double x = 0;
        double y = 0;
    };

protected:
    struct EventOptions
    {
        ParentJsType::EventOptions baseOptions;
        bool altKey = false;
        bool ctrlKey = false;
        bool shiftKey = false;
        bool metaKey = false;
        int32_t button = 0;
        int32_t buttons = 0;
        double screenX = 0;
        double screenY = 0;
        double clientX = 0;
        double clientY = 0;
        // TODO: think about adding relatedTarget field

        EventProperties ToDefaultProps() const;
    };

public:
    ~MouseEvent() override = default;

public:
    static JSObject* Constructor( JSContext* cx, const qwr::u8string& type, JS::HandleValue options = JS::UndefinedHandleValue );
    static JSObject* ConstructorWithOpt( JSContext* cx, size_t optArgCount, const qwr::u8string& type, JS::HandleValue options );

    bool get_AltKey() const;
    uint32_t get_Button() const;
    uint32_t get_Buttons() const;
    bool get_CtrlKey() const;
    bool get_MetaKey() const;
    JSObject* get_RelatedTarget() const;
    double get_ScreenX() const;
    double get_ScreenY() const;
    bool get_ShiftKey() const;
    double get_X() const;
    double get_Y() const;

protected:
    [[nodiscard]] MouseEvent( JSContext* cx, const qwr::u8string& type, const EventProperties& props );
    [[nodiscard]] MouseEvent( JSContext* cx, const qwr::u8string& type, const EventOptions& options = {} );
    [[nodiscard]] size_t GetInternalSize();

    [[nodiscard]] static EventOptions ExtractOptions( JSContext* cx, JS::HandleValue options );

private:
    static std::unique_ptr<MouseEvent> CreateNative( JSContext* cx, const qwr::u8string& type, const EventProperties& props );
    static std::unique_ptr<MouseEvent> CreateNative( JSContext* cx, const qwr::u8string& type, const EventOptions& options );

private:
    JSContext* pJsCtx_ = nullptr;

    EventProperties props_;
};

} // namespace mozjs
