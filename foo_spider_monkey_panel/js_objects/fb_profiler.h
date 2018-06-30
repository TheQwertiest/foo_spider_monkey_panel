#pragma once

#include <optional>
#include <string>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsFbProfiler
{
public:
    ~JsFbProfiler();

    static JSObject* Create( JSContext* cx, const std::string& name );

    static const JSClass& GetClass();

public:
    std::optional<std::nullptr_t> Print();
    std::optional<std::nullptr_t> Reset();

public:
    std::optional<uint32_t> get_Time();

private:
    JsFbProfiler( JSContext* cx, const std::string& name );
    JsFbProfiler( const JsFbProfiler& ) = delete;
    JsFbProfiler& operator=( const JsFbProfiler& ) = delete;

private:
    JSContext * pJsCtx_ = nullptr;
    pfc::string_simple name_;
    pfc::hires_timer timer_;
};

}
