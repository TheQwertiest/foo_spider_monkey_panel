#pragma once

#include <js_backend/objects/dom/event.h>

namespace smp
{
class MouseEventNew;
}

namespace mozjs
{

class MouseEvent
    : public JsObjectBase<MouseEvent>
    , private JsEvent
{
    friend class JsObjectBase<MouseEvent>;

private:
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = true;
    static constexpr bool HasParentProto = true;

    using BaseJsType = JsEvent;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId BasePrototypeId;
    static const JsPrototypeId ParentPrototypeId;
    static const JsPrototypeId PrototypeId;
    static const JSNative JsConstructor;

public:
    struct EventProperties
    {
        BaseJsType::EventProperties baseProps;
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
        BaseJsType::EventOptions baseOptions;
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
    double get_MovementX() const;
    double get_MovementY() const;
    double get_OffsetX() const;
    double get_OffsetY() const;
    JSObject* get_RelatedTarget() const;
    double get_ScreenX() const;
    double get_ScreenY() const;
    bool get_ShiftKey() const;
    double get_X() const;
    double get_Y() const;

protected:
    MouseEvent( JSContext* cx, const qwr::u8string& type, const EventProperties& props );
    MouseEvent( JSContext* cx, const qwr::u8string& type, const EventOptions& options = {} );
    [[nodiscard]] size_t GetInternalSize();

    static EventOptions ExtractOptions( JSContext* cx, JS::HandleValue options );

private:
    static std::unique_ptr<MouseEvent> CreateNative( JSContext* cx, const qwr::u8string& type, const EventProperties& props );
    static std::unique_ptr<MouseEvent> CreateNative( JSContext* cx, const qwr::u8string& type, const EventOptions& options );

private:
    JSContext* pJsCtx_ = nullptr;

    EventProperties props_;
};

} // namespace mozjs
