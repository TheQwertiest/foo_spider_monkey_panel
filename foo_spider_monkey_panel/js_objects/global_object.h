#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

class js_panel_window;

namespace mozjs
{

class JsContainer;

class JsGlobalObject
{
public:
    ~JsGlobalObject();

    static JSObject* Create( JSContext* cx, JsContainer &parentContainer, js_panel_window& parentPanel );

    static const JSClass& GetClass();

public:
    void Fail( std::string_view errorText);

    void AddHeapToTrace( JS::Heap<JS::Value>* heapValue );
    void RemoveHeapFromTrace( JS::Heap<JS::Value>* heapValue );

private:
    JsGlobalObject( JSContext* cx, JsContainer &parentContainer, js_panel_window& parentPanel );
    JsGlobalObject( const JsGlobalObject& ) = delete;    

    static void TraceHeapValue( JSTracer *trc, void *data );

private:
    JSContext * pJsCtx_;
    JsContainer &parentContainer_;
    js_panel_window& parentPanel_;

    std::unordered_map<JS::Heap<JS::Value>*, JS::Heap<JS::Value>*> tracerMap_;
};

}
