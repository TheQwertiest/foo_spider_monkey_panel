#pragma once

#include <js/TypeDecls.h>
#include <js/engine/js_gc.h>
#include <js/engine/js_monitor.h>
SMP_MJS_SUPPRESS_WARNINGS_PUSH
#include <js/Promise.h>
SMP_MJS_SUPPRESS_WARNINGS_POP

#include <functional>
#include <map>
#include <mutex>

namespace smp
{

class HeartbeatWindow;
}

namespace mozjs
{

class JsContainer;
class JsScriptCache;

class JsEngine final
{
public:
    ~JsEngine();
    JsEngine( const JsEngine& ) = delete;
    JsEngine& operator=( const JsEngine& ) = delete;

    static [[nodiscard]] JsEngine& GetInstance();
    void PrepareForExit();

public: // methods accessed by JsContainer
    [[nodiscard]] bool RegisterContainer( JsContainer& jsContainer );
    void UnregisterContainer( JsContainer& jsContainer );

    void MaybeRunJobs();

    void OnJsActionStart( JsContainer& jsContainer );
    void OnJsActionEnd( JsContainer& jsContainer );

public: // methods accessed by js objects
    [[nodiscard]] JsGc& GetGcEngine();
    [[nodiscard]] const JsGc& GetGcEngine() const;
    [[nodiscard]] JsScriptCache& GetScriptCache();

public: // methods accessed by other internals
    void OnHeartbeat();
    [[nodiscard]] bool OnInterrupt();

private:
    JsEngine();

private:
    bool Initialize();
    void Finalize();

    /// @throw qwr::QwrException
    void StartHeartbeatThread();
    void StopHeartbeatThread();

    static bool InterruptHandler( JSContext* cx );

    static void RejectedPromiseHandler( JSContext* cx, bool mutedErrors, JS::HandleObject promise,
                                        JS::PromiseRejectionHandlingState state,
                                        void* data );
    static JSObject* ModuleResolver( JSContext* cx, JS::HandleValue modulePrivate,
                                     JS::HandleObject moduleRequest );
    static bool ModuleMetaGenerator( JSContext* cx, JS::HandleValue modulePrivate,
                                     JS::HandleObject moduleRequest );

    void ReportOomError();

private:
    JSContext* pJsCtx_ = nullptr;

    bool isInitialized_ = false;
    bool shouldShutdown_ = false;

    std::map<void*, std::reference_wrapper<JsContainer>> registeredContainers_;

    bool isBeating_ = false;
    std::unique_ptr<smp::HeartbeatWindow> heartbeatWindow_;
    std::thread heartbeatThread_;
    std::atomic_bool shouldStopHeartbeatThread_ = false;

    JsGc jsGc_;
    JsMonitor jsMonitor_;

    JS::PersistentRooted<JS::GCVector<JSObject*, 0, js::SystemAllocPolicy>> rejectedPromises_;
    bool areJobsInProgress_ = false;
    uint32_t jobsStartTime_ = 0;

    std::unique_ptr<JsScriptCache> pScriptCache_;
};

} // namespace mozjs
