#pragma once

struct JSContext;
class JSObject;
class js_panel_window;

namespace mozjs
{

class JsGlobalObject
{
public:
    ~JsGlobalObject();

    static JSObject* Create( JSContext* cx, js_panel_window& parentPanel );
    
    void Fail( std::string_view errorText);

private:
    JsGlobalObject( JSContext* cx, js_panel_window& parentPanel );
    JsGlobalObject( const JsGlobalObject& ) = delete;    

private:
    JSContext * pJsCtx_;
    js_panel_window& parentPanel_;
};

}
