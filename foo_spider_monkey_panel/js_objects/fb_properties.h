#pragma once

#include <optional>
#include <map>
#include <vector>

class JSObject;
struct JSContext;
struct JSClass;

class js_panel_window;

namespace mozjs
{

class JsFbProperties
{
public:
    ~JsFbProperties();

    static JSObject* Create( JSContext* cx, js_panel_window& parentPanel );

    static const JSClass& GetClass();

public:
    void RemoveHeapTracer();

    std::optional<JS::Heap<JS::Value>> GetProperty( const std::string& propName, JS::HandleValue propDefaultValue );
    bool SetProperty( const std::string& propName, JS::HandleValue propValue );

private:
    JsFbProperties( JSContext* cx, js_panel_window& parentPanel );
    JsFbProperties( const JsFbProperties& ) = delete;
    JsFbProperties& operator=( const JsFbProperties& ) = delete;

    static void TraceHeapValue( JSTracer *trc, void *data );

private:
    JSContext * pJsCtx_ = nullptr;
    js_panel_window& parentPanel_;

    struct HeapElement
    {
        HeapElement( JS::HandleValue inValue )
            : value( inValue )
        {
        }
        JS::Heap<JS::Value> value;
    };
    std::unordered_map<std::string, std::shared_ptr<HeapElement>> properties_;
};

}
