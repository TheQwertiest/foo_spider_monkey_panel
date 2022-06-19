#pragma once

#include <js_objects/object_base.h>
#include <js_utils/js_fwd.h>

namespace mozjs
{

class JsEventTarget
    : public JsObjectBase<JsEventTarget>
{
public:
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = true;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
    static const JSNative JsConstructor;

public:
    ~JsEventTarget() override = default;

    static JSObject* Constructor( JSContext* cx );

public:
    static std::unique_ptr<JsEventTarget> CreateNative( JSContext* cx );
    static size_t GetInternalSize();

public:
    void AddEventListener( const qwr::u8string& type, JS::HandleValue listener );
    void RemoveEventListener( const qwr::u8string& type, JS::HandleValue listener );
    void DispatchEvent( JS::HandleValue event );

private:
    JsEventTarget( JSContext* cx );

private:
    [[maybe_unused]] JSContext* pJsCtx_ = nullptr;
};

} // namespace mozjs
