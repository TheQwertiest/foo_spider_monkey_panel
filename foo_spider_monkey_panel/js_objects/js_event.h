#pragma once

#include <js_objects/object_base.h>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsEvent
    : public JsObjectBase<JsEvent>
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
    ~JsEvent() override = default;

    static JSObject* Constructor( JSContext* cx, const qwr::u8string& type );

public:
    static std::unique_ptr<JsEvent> CreateNative( JSContext* cx, const qwr::u8string& type, uint64_t timeStamp );
    static size_t GetInternalSize( const qwr::u8string& type, uint64_t timeStamp );

public:
    uint64_t get_TimeStamp() const;
    const qwr::u8string& get_Type() const;

private:
    JsEvent( JSContext* cx, const qwr::u8string& type, uint64_t timeStamp );

private:
    [[maybe_unused]] JSContext* pJsCtx_ = nullptr;

    const qwr::u8string type_;
    const uint64_t timeStamp_;
};

} // namespace mozjs
