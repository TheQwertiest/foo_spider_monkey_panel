#pragma once

#include <js_objects/object_base.h>

namespace mozjs
{

class JsEventTarget
    : public JsObjectBase<JsEventTarget>
{
    friend class JsObjectBase<JsEventTarget>;

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
    void Trace( JSTracer* trc );
    void PrepareForGc();

public:
    static JSObject* Constructor( JSContext* cx );

    void AddEventListener( const qwr::u8string& type, JS::HandleValue listener );
    void RemoveEventListener( const qwr::u8string& type, JS::HandleValue listener );
    void DispatchEvent( JS::HandleValue event );

protected:
    JsEventTarget( JSContext* cx );
    static size_t GetInternalSize();

private:
    static std::unique_ptr<JsEventTarget> CreateNative( JSContext* cx );

private:
    [[maybe_unused]] JSContext* pJsCtx_ = nullptr;
    std::string test_;
};

} // namespace mozjs
