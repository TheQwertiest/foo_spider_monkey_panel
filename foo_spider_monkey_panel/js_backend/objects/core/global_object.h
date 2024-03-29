#pragma once

#include <js_backend/objects/core/script_loader.h>

#include <optional>
#include <unordered_set>

namespace smp::panel
{
class PanelWindow;
}

namespace mozjs
{

class JsContainer;
class JsWindow;
class GlobalHeapManager;

class JsGlobalObject
{
public:
    static constexpr bool HasProto = false;

    static const JSClass& JsClass;

public:
    // @remark No need to cleanup JS here, since it must be performed manually beforehand anyway
    ~JsGlobalObject() = default;

    static JSObject* CreateNative( JSContext* cx, JsContainer& parentContainer );
    static JsGlobalObject* ExtractNative( JSContext* cx, JS::HandleObject jsObject );

public:
    void Fail( const qwr::u8string& errorText );

    [[nodiscard]] GlobalHeapManager& GetHeapManager() const;

    static void PrepareForGc( JSContext* cx, JS::HandleObject self );

    ScriptLoader& GetScriptLoader();

    /// @remark HWND might be null, if called before fb2k initialization is completed
    [[nodiscard]] HWND GetPanelHwnd() const;

public:
    void ClearInterval( uint32_t intervalId );
    void ClearTimeout( uint32_t timeoutId );

    void IncludeScript( const qwr::u8string& path, JS::HandleValue options = JS::UndefinedHandleValue );
    void IncludeScriptWithOpt( size_t optArgCount, const qwr::u8string& path, JS::HandleValue options );
    uint32_t SetInterval( JS::HandleValue func, uint32_t delay, JS::HandleValueArray funcArgs = JS::HandleValueArray{ JS::UndefinedHandleValue } );
    uint32_t SetIntervalWithOpt( size_t optArgCount, JS::HandleValue func, uint32_t delay, JS::HandleValueArray funcArgs );
    uint32_t SetTimeout( JS::HandleValue func, uint32_t delay, JS::HandleValueArray funcArgs );
    uint32_t SetTimeoutWithOpt( size_t optArgCount, JS::HandleValue func, uint32_t delay, JS::HandleValueArray funcArgs );

private:
    JsGlobalObject( JSContext* cx, JsContainer& parentContainer, JsWindow* pJsWindow );

    struct IncludeOptions
    {
        bool alwaysEvaluate = false;
    };

    IncludeOptions ParseIncludeOptions( JS::HandleValue options );

    static void Trace( JSTracer* trc, JSObject* obj );

private:
    JSContext* pJsCtx_ = nullptr;
    JsContainer& parentContainer_;

    JsWindow* pJsWindow_ = nullptr;

    std::unique_ptr<GlobalHeapManager> heapManager_;

    ScriptLoader scriptLoader_;
};

} // namespace mozjs
