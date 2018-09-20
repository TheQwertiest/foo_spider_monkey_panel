#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <js_engine/native_to_js_invoker.h>

#include <optional>


class js_panel_window;

namespace mozjs
{

class JsEngine;
class JsGlobalObject;
class JsGdiGraphics;
class JsDropSourceAction;
struct DropActionParams;

class JsContainer final
{
    // To access preparation call
    friend class JsEngine;

public:
    JsContainer();
    ~JsContainer();

public:
    enum class JsStatus
    {
        Ready,
        Prepared,
        NotPrepared,
        Failed
    };

public:
    bool Initialize();
    void Finalize();

    void Fail( const pfc::string8_fast &errorText );

    JsStatus GetStatus() const;

    bool ExecuteScript( const pfc::string8_fast& scriptCode );

    template <typename ReturnType = std::nullptr_t, typename... ArgTypes>
    std::optional<ReturnType> InvokeJsCallback( pfc::string8_fast functionName,
                                                ArgTypes&&... args )
    {
        if ( JsStatus::Ready != jsStatus_ )
        {
            return std::nullopt;
        }
        return mozjs::InvokeJsCallback<ReturnType>( pJsCtx_, jsGlobal_, functionName, std::forward<ArgTypes>( args )... );
    }

public: // callbacks that require js data
    void InvokeOnNotify( WPARAM wp, LPARAM lp );
    void InvokeOnPaint( Gdiplus::Graphics& gr );
    void InvokeOnDragAction( const pfc::string8_fast& functionName, const POINTL& pt, uint32_t keyState, DropActionParams& actionParams );

    uint32_t SetInterval( HWND hWnd, uint32_t delay, JS::HandleFunction jsFunction );
    uint32_t SetTimeout( HWND hWnd, uint32_t delay, JS::HandleFunction jsFunction );
    void KillTimer( uint32_t timerId );
    void InvokeTimerFunction( uint32_t timerId );

private:
    JsContainer( const JsContainer& ) = delete;

    bool Prepare( JSContext *cx, js_panel_window& parentPanel );

    bool CreateDropActionIfNeeded();

private:
    JSContext * pJsCtx_ = nullptr;
    js_panel_window* pParentPanel_ = nullptr;

    JS::PersistentRootedObject jsGlobal_;
    JS::PersistentRootedObject jsGraphics_;
    JS::PersistentRootedObject jsDropAction_;
    JsGlobalObject* pNativeGlobal_ = nullptr;
    JsGdiGraphics* pNativeGraphics_ = nullptr;
    JsDropSourceAction* pNativeDropAction_ = nullptr;

    JsStatus jsStatus_ = JsStatus::NotPrepared;
};

}
