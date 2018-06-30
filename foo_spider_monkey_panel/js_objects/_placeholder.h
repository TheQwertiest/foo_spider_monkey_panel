#pragma once

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsPlaceholder
{
public:
    ~JsPlaceholder();

    static JSObject* Create( JSContext* cx );

    static const JSClass& GetClass();

public:


private:
    JsPlaceholder( JSContext* cx );
    JsPlaceholder( const JsPlaceholder& ) = delete;
    JsPlaceholder& operator=( const JsPlaceholder& ) = delete;

private:
    JSContext * pJsCtx_ = nullptr;
};

}
