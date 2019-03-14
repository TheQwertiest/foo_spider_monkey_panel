#pragma once

#include <js_engine/js_gc.h>

#include <map>
#include <functional>
#include <mutex>

class js_panel_window;
struct JSContext;

namespace smp
{

class HeartbeatWindow;
}

namespace mozjs
{

class JsContainer;
class JsInternalGlobal;

class JsEngine final
{
public:
    ~JsEngine();
    JsEngine( const JsEngine& ) = delete;
    JsEngine& operator=( const JsEngine& ) = delete;

    static JsEngine& GetInstance();
    void PrepareForExit();

public:
    bool RegisterContainer( JsContainer& jsContainer );
    void UnregisterContainer( JsContainer& jsContainer );

public:
    const JsGc& GetGcEngine() const;
    JsInternalGlobal& GetInternalGlobal();

public:
    void OnHeartbeat();
    void MaybeRunJobs();

private:
    JsEngine();

private:
    bool Initialize();
    void Finalize();

    /// @throw smp::SmpException
    void StartHeartbeatThread();
    void StopHeartbeatThread();

    static void RejectedPromiseHandler( JSContext* cx, JS::HandleObject promise,
                                        JS::PromiseRejectionHandlingState state,
                                        void* data );

private:
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

    JS::PersistentRooted<JS::GCVector<JSObject*, 0, js::SystemAllocPolicy>> rejectedPromises_;
    bool areJobsInProgress_ = false;
    uint32_t jobsStartTime_ = 0;

    std::unique_ptr<JsInternalGlobal> internalGlobal_;
};

} // namespace mozjs
