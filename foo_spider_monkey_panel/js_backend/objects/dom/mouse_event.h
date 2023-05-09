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

protected:
    struct EventOptions
    {
        BaseJsType::EventOptions baseOptions;
        double screenX = 0;
        double screenY = 0;
        double clientX = 0;
        double clientY = 0;
        bool ctrlKey = false;
        bool shiftKey = false;
        bool altKey = false;
        bool metaKey = false;
        int32_t button = 0;
        int32_t buttons = 0;
        // TODO: think about adding relatedTarget field
    };

public:
    ~MouseEvent() override = default;

public:
    static JSObject* Constructor( JSContext* cx, const qwr::u8string& type, JS::HandleValue options = JS::UndefinedHandleValue );
    static JSObject* ConstructorWithOpt( JSContext* cx, size_t optArgCount, const qwr::u8string& type, JS::HandleValue options );

    uint8_t get_Reason();

protected:
    MouseEvent( JSContext* cx, const qwr::u8string& type, const EventOptions& options = {} );
    [[nodiscard]] size_t GetInternalSize();

    static EventOptions ExtractOptions( JSContext* cx, JS::HandleValue options );

private:
    static std::unique_ptr<MouseEvent> CreateNative( JSContext* cx, const qwr::u8string& type, const smp::MouseEventNew& event );
    static std::unique_ptr<MouseEvent> CreateNative( JSContext* cx, const qwr::u8string& type, const EventOptions& options );

private:
    JSContext* pJsCtx_ = nullptr;

    play_control::t_stop_reason stopReason_ = play_control::stop_reason_user;
};

} // namespace mozjs
