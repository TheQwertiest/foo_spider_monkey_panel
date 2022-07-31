#pragma once

#include <js_objects/object_base.h>
#include <js_utils/js_fwd.h>

namespace mozjs
{

class JsEvent
    : public JsObjectBase<JsEvent>
{
    friend class JsObjectBase<JsEvent>;

public:
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = true;
    static constexpr bool IsExtendable = true;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
    static const JSNative JsConstructor;

public:
    ~JsEvent() override;

public:
    bool IsPropagationStopped() const;
    bool HasTarget() const;
    void SetCurrentTarget( JS::HandleObject pTarget );
    void ResetPropagationStatus();

public:
    static JSObject* Constructor( JSContext* cx, const qwr::u8string& type, JS::HandleValue options = JS::UndefinedHandleValue );
    static JSObject* ConstructorWithOpt( JSContext* cx, size_t optArgCount, const qwr::u8string& type, JS::HandleValue options );

    void DoNothing() const;
    JS::Value ComposedPath() const;
    void StopImmediatePropagation();
    void PreventDefault();

    bool get_False() const;
    bool get_Cancelable() const;
    bool get_DefaultPrevented() const;
    uint8_t get_EventPhase() const;
    JSObject* get_Target() const;
    uint64_t get_TimeStamp() const;
    const qwr::u8string& get_Type() const;

protected:
    JsEvent( JSContext* cx, const qwr::u8string& type, bool isCancelable );
    [[nodiscard]] size_t GetInternalSize();

private:
    static std::unique_ptr<JsEvent> CreateNative( JSContext* cx, const qwr::u8string& type, bool isCancelable );

private:
    [[maybe_unused]] JSContext* pJsCtx_ = nullptr;

    JS::PersistentRootedObject currentTarget_;

    bool isCancelable_;
    const qwr::u8string type_;
    const uint64_t timeStamp_;

    bool isDefaultPrevented_ = false;
    bool isPropagationStopped_ = false;
};

} // namespace mozjs
