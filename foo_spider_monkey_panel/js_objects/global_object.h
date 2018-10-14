#pragma once

#include <optional>
#include <set>

class js_panel_window;

namespace mozjs
{

class JsContainer;
class GlobalHeapManager;

class JsGlobalObject
{
public:
    static constexpr bool HasProto = false;
    static constexpr bool HasProxy = false;
    static constexpr bool HasPostCreate = false;

    static const JSClass& JsClass;

public:
    ~JsGlobalObject();

    static JSObject* CreateNative( JSContext* cx, JsContainer& parentContainer, js_panel_window& parentPanel );

public:
    void Fail( const pfc::string8_fast& errorText );

    GlobalHeapManager& GetHeapManager() const;

    static void CleanupBeforeDestruction( JSContext* cx, JS::HandleObject self );

public: // methods
    void IncludeScript( const pfc::string8_fast& path );

    template <typename T>
    static void CleanupObjectProperty( JSContext* cx, JS::HandleObject self, const std::string& propName )
    {
        JS::RootedValue jsPropertyValue( cx );
        if ( JS_GetProperty( cx, self, propName.data(), &jsPropertyValue ) && jsPropertyValue.isObject() )
        {
            JS::RootedObject jsProperty( cx, &jsPropertyValue.toObject() );
            auto pNative = static_cast<T*>( JS_GetInstancePrivate( cx, jsProperty, &T::JsClass, nullptr ) );
            if ( pNative )
            {
                pNative->CleanupBeforeDestruction();
            }
        }
    }

private:
    JsGlobalObject( JSContext* cx, JsContainer& parentContainer );

private:
    JSContext* pJsCtx_ = nullptr;
    JsContainer& parentContainer_;

private: // heap
    std::unique_ptr<GlobalHeapManager> heapManager_;
};

} // namespace mozjs
