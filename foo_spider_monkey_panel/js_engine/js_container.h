#pragma once

#include <js_engine/native_to_js_invoker.h>

#include <qwr/final_action.h>

#include <filesystem>
#include <optional>

class HostTimerTask;

namespace smp::panel
{
class js_panel_window;
struct DragActionParams;
} // namespace smp::panel

namespace mozjs
{

class JsEngine;
class JsRealmInner;
class JsGlobalObject;
class JsGdiGraphics;
class JsDropSourceAction;
class JsAsyncTask;

// Must not leak exceptions!
class JsContainer final
    : public std::enable_shared_from_this<JsContainer>
{
    // To set JS_Context
    friend class JsEngine;

public:
    JsContainer( smp::panel::js_panel_window& parentPanel );
    JsContainer( const JsContainer& ) = delete;
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
    [[nodiscard]] bool Initialize();
    void Finalize();

    void Fail( const qwr::u8string& errorText );

    [[nodiscard]] JsStatus GetStatus() const;

    [[nodiscard]] bool ExecuteScript( const qwr::u8string& scriptCode );
    [[nodiscard]] bool ExecuteScriptFile( const std::filesystem::path& scriptPath );

    static void RunJobs();

public:
    smp::panel::js_panel_window& GetParentPanel() const;

public:
    template <typename ReturnType = std::nullptr_t, typename... ArgTypes>
    std::optional<ReturnType> InvokeJsCallback( qwr::u8string functionName,
                                                ArgTypes&&... args )
    {
        if ( !IsReadyForCallback() )
        {
            return std::nullopt;
        }

        auto selfSaver = shared_from_this();

        OnJsActionStart();
        qwr::final_action autoAction( [&] { OnJsActionEnd(); } );

        return mozjs::InvokeJsCallback<ReturnType>( pJsCtx_, jsGlobal_, functionName, std::forward<ArgTypes>( args )... );
    }

    [[nodiscard]] bool InvokeOnDragAction( const qwr::u8string& functionName, const POINTL& pt, uint32_t keyState, smp::panel::DragActionParams& actionParams );
    void InvokeOnNotify( const std::wstring& name, JS::HandleValue info );
    void InvokeOnPaint( Gdiplus::Graphics& gr );
    bool InvokeJsAsyncTask( JsAsyncTask& jsTask );

private:
    void SetJsCtx( JSContext* cx );

    [[nodiscard]] bool IsReadyForCallback() const;

    /// @return true on success, false with JS report on failure
    [[nodiscard]] bool CreateDropActionIfNeeded();

    void OnJsActionStart();
    void OnJsActionEnd();

private:
    JSContext* pJsCtx_ = nullptr;
    smp::panel::js_panel_window* pParentPanel_ = nullptr;

    JS::PersistentRootedObject jsGlobal_;
    JS::PersistentRootedObject jsGraphics_;
    JS::PersistentRootedObject jsDropAction_;

    JsRealmInner* pNativeRealm_ = nullptr;
    JsGlobalObject* pNativeGlobal_ = nullptr;
    JsGdiGraphics* pNativeGraphics_ = nullptr;
    JsDropSourceAction* pNativeDropAction_ = nullptr;

    JsStatus jsStatus_ = JsStatus::EngineFailed;
    bool isParsingScript_ = false;
    uint32_t nestedJsCounter_ = 0;
};

} // namespace mozjs
