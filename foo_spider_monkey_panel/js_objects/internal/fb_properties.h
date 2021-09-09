#pragma once

#include <map>
#include <optional>
#include <vector>

class JSObject;
struct JSContext;
struct JSClass;

namespace smp::panel
{
class js_panel_window;
}

namespace mozjs
{

class FbProperties
{
public:
    FbProperties( const FbProperties& ) = delete;
    FbProperties& operator=( const FbProperties& ) = delete;
    ~FbProperties() = default;

    static [[nodiscard]] std::unique_ptr<FbProperties> Create( JSContext* cx, smp::panel::js_panel_window& parentPanel );

public:
    void Trace( JSTracer* trc );
    void PrepareForGc();

    [[nodiscard]] JS::Value GetProperty( const std::wstring& propName, JS::HandleValue propDefaultValue );
    void SetProperty( const std::wstring& propName, JS::HandleValue propValue );

private:
    FbProperties( JSContext* cx, smp::panel::js_panel_window& parentPanel );

private:
    JSContext* pJsCtx_ = nullptr;
    smp::panel::js_panel_window& parentPanel_;

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
