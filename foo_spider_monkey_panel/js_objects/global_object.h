#pragma once

struct JSContext;
class JSObject;

class js_panel_window;

namespace mozjs
{

class JsContainer;

class JsGlobalObject
{
public:
    ~JsGlobalObject();

    static JSObject* Create( JSContext* cx, JsContainer &parentContainer, js_panel_window& parentPanel );
    
    void Fail( std::string_view errorText);

private:
    JsGlobalObject( JSContext* cx, JsContainer &parentContainer, js_panel_window& parentPanel );
    JsGlobalObject( const JsGlobalObject& ) = delete;    

private:
    JSContext * pJsCtx_;
    JsContainer &parentContainer_;
    js_panel_window& parentPanel_;
};

}
