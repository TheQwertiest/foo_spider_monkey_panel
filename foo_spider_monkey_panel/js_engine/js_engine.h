#pragma once

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

class JsEngine final
{
public:
    ~JsEngine();
    JsEngine( const JsEngine& ) = delete;
    JsEngine& operator=( const JsEngine& ) = delete;

    static JsEngine& GetInstance();
    void PrepareForExit();

public:
    bool RegisterPanel( js_panel_window& parentPanel, JsContainer& jsContainer );
    void UnregisterPanel( js_panel_window& parentPanel );

public:
    void OnHeartbeat();

private:
    JsEngine();

private:
    bool Initialize();
    void Finalize();

    bool StartHeartbeatThread();
    void StopHeartbeatThread();

private:
    enum class GcLevel: uint8_t
    {
        Incremental,
        Normal,
        Full
    };

    void MaybeGc();
    void MaybeRunJobs();

    uint64_t GetCurrentTotalHeapSize();
    uint64_t GetCurrentTotalAllocCount();
    void PerformGc( GcLevel gcLevel );
    void PerformIncrementalGc();
    void PerformNormalGc();
    void PerformFullGc();
    void PrepareCompartmentsForGc( GcLevel gcLevel );
    void NotifyCompartmentsOnGcEnd();

private:
    JSContext * pJsCtx_ = nullptr;

    bool isInitialized_ = false;
    bool shouldShutdown_ = false;

    std::map<HWND, std::reference_wrapper<JsContainer>> registeredContainers_;

    std::unique_ptr<smp::HeartbeatWindow> heartbeatWindow_;
    std::thread heartbeatThread_;
    std::atomic_bool shouldStopHeartbeatThread_ = false;    
    
    uint32_t lastGcCheckTime_ = 0;
    uint64_t lastTotalHeapSize_ = 0;
    uint64_t lastTotalAllocCount_ = 0;

    uint32_t maxHeapSize_ = 1024UL * 1024 * 1024;
    uint32_t heapGrowthRateTrigger_ = 50UL * 1024 * 1024;
    uint32_t gcSliceTimeBudget_ = 30;
    uint32_t gcCheckDelay_ = 50;
    uint32_t allocCountTrigger_ = 50;

    bool areJobsInProgress_ = false;
    uint32_t jobsStartTime_ = 0;
};

}
