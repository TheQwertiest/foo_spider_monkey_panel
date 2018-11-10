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
    static constexpr bool HasGlobalProto = true;
    static constexpr bool HasProxy = false;
    static constexpr bool HasPostCreate = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
    static const JSNative JsConstructor;

public:
    ~JsFbProfiler();

    static std::unique_ptr<JsFbProfiler> CreateNative( JSContext* cx, const pfc::string8_fast& name );
    static size_t GetInternalSize( const pfc::string8_fast& name );

public: // ctor
    static JSObject* Constructor( JSContext* cx, const pfc::string8_fast& name );

public:
    void Print( const pfc::string8_fast& additionalMsg = "", bool printComponentInfo = true );
    void PrintWithOpt( size_t optArgCount, const pfc::string8_fast& additionalMsg, bool printComponentInfo );
    void Reset();

public:
    uint32_t get_Time();

private:
    JsFbProfiler( JSContext* cx, const pfc::string8_fast& name );

private:
    JSContext* pJsCtx_ = nullptr;
    pfc::string_simple name_;
    pfc::hires_timer timer_;
};

} // namespace mozjs
