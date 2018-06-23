#pragma once

struct JSContext;
class JSObject;

namespace mozjs
{

class JsGlobalObject
{
public:
    ~JsGlobalObject();

    static JSObject* Create( JSContext* cx, HWND hParentPanel );

    HWND GetHWND() const;
    bool HasFailed() const;
    void Fail();

private:
    JsGlobalObject( JSContext* cx, HWND hParentPanel );
    JsGlobalObject( const JsGlobalObject& ) = delete;    

private:
    JSContext * pJsCtx_;
    HWND hParentPanel_;
    bool hasFailed_;
};

}
