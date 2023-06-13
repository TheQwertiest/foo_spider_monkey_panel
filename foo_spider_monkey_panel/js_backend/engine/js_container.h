#pragma once

#include <js_backend/engine/js_event_status.h>
#include <js_backend/engine/native_to_js_invoker.h>
#include <panel/panel_fwd.h>

#include <qwr/final_action.h>

#include <filesystem>
#include <optional>

namespace smp
{
class EventBase;

namespace panel
{
struct DragActionParams;
} // namespace panel

} // namespace smp

namespace mozjs
{

class ContextInner;
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
    friend class ContextInner;

public:
    JsContainer( smp::not_null_shared<smp::panel::PanelAccessor> pHostPanel );
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

    [[nodiscard]] bool ExecuteScript( const qwr::u8string& scriptCode, bool isModule );
    [[nodiscard]] bool ExecuteScriptFile( const std::filesystem::path& scriptPath, bool isModule );

    static void PerformMicroTaskCheckPoint();

public:
    smp::not_null_shared<smp::panel::PanelAccessor> GetHostPanel() const;

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
    bool InvokeJsAsyncTask( JsAsyncTask& jsTask );

    EventStatus InvokeJsEvent( smp::EventBase& event );

private:
    void SetJsCtx( JSContext* cx );

    [[nodiscard]] bool IsReadyForCallback() const;

    /// @return true on success, false with JS report on failure
    [[nodiscard]] bool CreateDropActionIfNeeded();

    void OnJsActionStart();
    void OnJsActionEnd();

private:
    smp::not_null_shared<smp::panel::PanelAccessor> pHostPanel_;
    JSContext* pJsCtx_ = nullptr;

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
