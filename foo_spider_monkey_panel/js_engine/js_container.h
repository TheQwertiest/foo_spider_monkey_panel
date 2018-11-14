#pragma once

#include <js_engine/native_to_js_invoker.h>

#include <optional>

namespace smp::panel
{
class js_panel_window;
struct DropActionParams;
} // namespace smp::panel

namespace mozjs
{

class JsEngine;
class JsGlobalObject;
class JsGdiGraphics;
class JsDropSourceAction;

// Must not leak exceptions!
class JsContainer final
    : public std::enable_shared_from_this<JsContainer>
{
    // To set JS_Context
    friend class JsEngine;

public:
    JsContainer( smp::panel::js_panel_window& parentPanel );
    ~JsContainer();

public:
    enum class JsStatus
    {
        EngineFailed,
        Failed,
        Ready,
        Working
    };

public:
    bool Initialize();
    void Finalize();

    void Fail( const pfc::string8_fast& errorText );

    JsStatus GetStatus() const;

    bool ExecuteScript( const pfc::string8_fast& scriptCode );

    template <typename ReturnType = std::nullptr_t, typename... ArgTypes>
    std::optional<ReturnType> InvokeJsCallback( pfc::string8_fast functionName,
                                                ArgTypes&&... args )
    {
        if ( !IsReadyForCallback() )
        {
            return std::nullopt;
        }

        auto selfSaver = shared_from_this();
        return mozjs::InvokeJsCallback<ReturnType>( pJsCtx_, jsGlobal_, functionName, std::forward<ArgTypes>( args )... );
    }

    static void RunMicroTasks();

public: // callbacks that require js data
    void InvokeOnDragAction( const pfc::string8_fast& functionName, const POINTL& pt, uint32_t keyState, smp::panel::DropActionParams& actionParams );
    void InvokeOnNotify( WPARAM wp, LPARAM lp );
    void InvokeOnPaint( Gdiplus::Graphics& gr );
    void InvokeTimerFunction( uint32_t timerId );

private:
    JsContainer( const JsContainer& ) = delete;

    void SetJsCtx( JSContext* cx );

    bool IsReadyForCallback() const;

    /// @return true on success, false with JS report on failure
    bool CreateDropActionIfNeeded();

private:
    JSContext* pJsCtx_ = nullptr;
    smp::panel::js_panel_window* pParentPanel_ = nullptr;

    JS::PersistentRootedObject jsGlobal_;
    JS::PersistentRootedObject jsGraphics_;
    JS::PersistentRootedObject jsDropAction_;
    JsGlobalObject* pNativeGlobal_ = nullptr;
    JsGdiGraphics* pNativeGraphics_ = nullptr;
    JsDropSourceAction* pNativeDropAction_ = nullptr;

    JsStatus jsStatus_ = JsStatus::EngineFailed;
    bool isParsingScript_ = false;
};

} // namespace mozjs
