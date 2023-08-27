#pragma once

#include <js_backend/engine/js_gc.h>
#include <js_backend/engine/js_monitor.h>

#include <js/TypeDecls.h>
SMP_MJS_SUPPRESS_WARNINGS_PUSH
#include <js/Promise.h>
SMP_MJS_SUPPRESS_WARNINGS_POP

#include <functional>
#include <mutex>
#include <queue>
#include <unordered_map>

namespace
{
class SavedQueue;
}

namespace smp
{
class MicroTaskRunnable;
class HeartbeatWindow;
} // namespace smp

namespace mozjs
{

class JsContainer;
class JsScriptCache;
class JsEngine;

class ContextInner final : private JS::JobQueue
{
    friend class AutoJsAction;
    friend class JsEngine;
    friend class ::SavedQueue;

public:
    ~ContextInner();
    ContextInner( const ContextInner& ) = delete;
    ContextInner& operator=( const ContextInner& ) = delete;

    static [[nodiscard]] ContextInner& Get();
    void PrepareForExit();

public: // methods accessed by JsContainer
    [[nodiscard]] bool RegisterContainer( JsContainer& jsContainer );
    void UnregisterContainer( JsContainer& jsContainer );

    void OnJsActionStart( JsContainer& jsContainer );
    void OnJsActionEnd( JsContainer& jsContainer );

    void EnqueueMicroTask( smp::not_null_shared<smp::MicroTaskRunnable> pMicroTask );
    void PerformMicroTaskCheckPoint();

public: // methods accessed by js objects
    [[nodiscard]] JsGc& GetGcEngine();
    [[nodiscard]] const JsGc& GetGcEngine() const;
    [[nodiscard]] JsScriptCache& GetScriptCache();

public: // methods accessed by other internals
    void OnHeartbeat();

private:
    ContextInner();

private: // JS::JobQueue
    JSObject* getIncumbentGlobal( JSContext* cx ) final;
    bool enqueuePromiseJob( JSContext* cx, JS::HandleObject promise, JS::HandleObject job, JS::HandleObject allocationSite, JS::HandleObject incumbentGlobal ) final;
    void runJobs( JSContext* cx ) final;
    bool empty() const final;
    js::UniquePtr<SavedJobQueue> saveJobQueue( JSContext* ) final;
    void ClearJobQueue();

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

    // TODO: move module handling to a separate file
    static JSObject* ModuleResolver( JSContext* cx, JS::HandleValue modulePrivate,
                                     JS::HandleObject moduleRequest );
    static bool ModuleMetaGenerator( JSContext* cx, JS::HandleValue modulePrivate,
                                     JS::HandleObject moduleRequest );

    void ReportOomError();

private:
    // TODO: separate engine from context
    // TODO: move all js globals to engine or context
    JSContext* pJsCtx_ = nullptr;

    bool isInitialized_ = false;

    std::unordered_map<void*, std::reference_wrapper<JsContainer>> registeredContainers_;

    bool isBeating_ = false;
    std::unique_ptr<smp::HeartbeatWindow> heartbeatWindow_;
    std::unique_ptr<std::jthread> pHeartbeatThread_;

    JsGc jsGc_;
    JsMonitor jsMonitor_;

    JS::PersistentRooted<JS::GCVector<JSObject*, 0, js::SystemAllocPolicy>> rejectedPromises_;
    std::queue<smp::not_null_shared<smp::MicroTaskRunnable>> microTaskQueue_;
    bool inMicroTaskCheckpoint_ = false;
    bool isDrainingMicroTasks_ = false;
    bool isRunMicroTasksInterrupted_ = false;
    uint32_t jobsStartTime_ = 0;

    std::unique_ptr<JsScriptCache> pScriptCache_;
};

} // namespace mozjs
