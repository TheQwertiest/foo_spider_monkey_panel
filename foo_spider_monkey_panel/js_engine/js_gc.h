#pragma once

#include <stdint.h>

namespace mozjs
{

class JsGc final
{
public:
    JsGc() = default;
    ~JsGc() = default;
    JsGc( const JsGc& ) = delete;
    JsGc& operator=( const JsGc& ) = delete;

public:    
    static uint32_t GetMaxHeap() noexcept(false);

    void Initialize( JSContext* pJsCtx ) noexcept(false);  

    bool MaybeGc();

private:
    enum class GcLevel: uint8_t
    {
        None,
        Incremental,
        Normal,
        Full
    };

    static void UpdateGcConfig();
    
    // GC stats handling
    bool IsTimeToGc();
    GcLevel GetRequiredGcLevel();
    GcLevel GetGcLevelFromHeapSize();
    GcLevel GetGcLevelFromAllocCount();
    uint64_t GetCurrentTotalHeapSize();
    uint64_t GetCurrentTotalAllocCount();
    void UpdateGcStats();

    // GC implementation
    void PerformGc( GcLevel gcLevel );
    void PerformIncrementalGc();
    void PerformNormalGc();
    void PerformFullGc();
    void PrepareCompartmentsForGc( GcLevel gcLevel );
    void NotifyCompartmentsOnGcEnd();

private:
    JSContext * pJsCtx_ = nullptr;
    
    uint32_t lastGcCheckTime_ = 0;
    uint64_t lastTotalHeapSize_ = 0;
    uint64_t lastTotalAllocCount_ = 0;

    // These values are overwritten by config.
    // Remain here mostly as a reference.
    uint32_t maxHeapSize_ = 1024UL * 1024 * 1024;
    uint32_t heapGrowthRateTrigger_ = 50UL * 1024 * 1024;
    uint32_t gcSliceTimeBudget_ = 30;
    uint32_t gcCheckDelay_ = 50;
    uint32_t allocCountTrigger_ = 50;
};

}
