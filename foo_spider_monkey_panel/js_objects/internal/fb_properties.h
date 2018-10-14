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

class FbProperties
{
public:
    FbProperties( const FbProperties& ) = delete;
    FbProperties& operator=( const FbProperties& ) = delete;
    ~FbProperties();

    static std::unique_ptr<FbProperties> Create( JSContext* cx, js_panel_window& parentPanel );

public:
    void RemoveHeapTracer();

    JS::Value GetProperty( const std::wstring& propName, JS::HandleValue propDefaultValue );
    void SetProperty( const std::wstring& propName, JS::HandleValue propValue );

private:
    FbProperties( JSContext* cx, js_panel_window& parentPanel );

    static void TraceHeapValue( JSTracer* trc, void* data );

private:
    JSContext* pJsCtx_ = nullptr;
    js_panel_window& parentPanel_;

    struct HeapElement
    {
        HeapElement( JS::HandleValue inValue )
            : value( inValue )
        {
        }
        JS::Heap<JS::Value> value;
    };
    std::unordered_map<std::wstring, std::unique_ptr<HeapElement>> properties_;
};

} // namespace mozjs
