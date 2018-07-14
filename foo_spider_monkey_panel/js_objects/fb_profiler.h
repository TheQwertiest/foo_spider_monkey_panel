#pragma once

#include <js_objects/object_base.h>

#include <optional>
#include <string>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsFbProfiler
    : public JsObjectBase<JsFbProfiler>
{
public:
    static constexpr bool HasProto = true;
    // TODO: add global proto
    static constexpr bool HasGlobalProto = false;
    static constexpr bool HasProxy = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;

public:
    ~JsFbProfiler();

    static std::unique_ptr<JsFbProfiler> CreateNative( JSContext* cx, const pfc::string8_fast& name );
    static size_t GetInternalSize( const pfc::string8_fast& name );

public:
    // TODO: add a new argument to print (custom message) and update doc
    std::optional<std::nullptr_t> Print();
    std::optional<std::nullptr_t> Reset();

public:
    std::optional<uint32_t> get_Time();

private:
    JsFbProfiler( JSContext* cx, const pfc::string8_fast& name );

private:
    JSContext * pJsCtx_ = nullptr;
    pfc::string_simple name_;
    pfc::hires_timer timer_;
};

}
