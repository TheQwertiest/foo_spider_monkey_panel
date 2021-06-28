#pragma once

#include <optional>
#include <unordered_set>

namespace smp::panel
{
class js_panel_window;
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
    static constexpr bool HasProxy = false;
    static constexpr bool HasPostCreate = false;

    static const JSClass& JsClass;

public:
    // @remark No need to cleanup JS here, since it must be performed manually beforehand anyway
    ~JsGlobalObject() = default;

    static JSObject* CreateNative( JSContext* cx, JsContainer& parentContainer );

public:
    void Fail( const qwr::u8string& errorText );

    [[nodiscard]] GlobalHeapManager& GetHeapManager() const;

    static void PrepareForGc( JSContext* cx, JS::HandleObject self );

public: // methods
    /// @remark HWND might be null, if called before fb2k initialization is completed
    [[nodiscard]] HWND GetPanelHwnd() const;

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

    template <typename T>
    static T* GetNativeObjectProperty( JSContext* cx, JS::HandleObject self, const std::string& propName )
    {
        JS::RootedValue jsPropertyValue( cx );
        if ( JS_GetProperty( cx, self, propName.data(), &jsPropertyValue ) && jsPropertyValue.isObject() )
        {
            JS::RootedObject jsProperty( cx, &jsPropertyValue.toObject() );
            return static_cast<T*>( JS_GetInstancePrivate( cx, jsProperty, &T::JsClass, nullptr ) );
        }

        return nullptr;
    }

    template <typename T>
    static void CleanupObjectProperty( JSContext* cx, JS::HandleObject self, const std::string& propName )
    {
        auto pNative = GetNativeObjectProperty<T>( cx, self, propName );
        if ( pNative )
        {
            pNative->PrepareForGc();
        }
    }

    static void Trace( JSTracer* trc, JSObject* obj );

private:
    JSContext* pJsCtx_ = nullptr;
    JsContainer& parentContainer_;

    JsWindow* pJsWindow_ = nullptr;

    std::unordered_set<std::string> includedFiles_;

    std::unique_ptr<GlobalHeapManager> heapManager_;
};

} // namespace mozjs
