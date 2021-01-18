#include <stdafx.h>

#include "js_gc.h"

#include <fb2k/advanced_config.h>
#include <js_engine/js_container.h>
#include <js_engine/js_realm_inner.h>
#include <js_utils/js_error_helper.h>

#include <qwr/winapi_error_helpers.h>

namespace
{

constexpr uint32_t kDefaultHeapMaxMb = 1024L * 1024 * 1024;
constexpr uint32_t kDefaultHeapThresholdMb = 50L * 1024 * 1024;
constexpr uint32_t kHighFreqTimeLimitMs = 1000;
constexpr uint32_t kHighFreqBudgetMultiplier = 2;
constexpr uint32_t kHighFreqHeapGrowthMultiplier = 2;

} // namespace

namespace mozjs
{

uint32_t JsGc::GetMaxHeap()
{
    namespace smp_advconf = smp::config::advanced;

    UpdateGcConfig();

    return smp_advconf::gc_max_heap.GetValue();
}

uint64_t JsGc::GetTotalHeapUsageForGlobal( JSContext*, JS::HandleObject jsGlobal )
{
    assert( jsGlobal );

    auto pJsRealm = static_cast<JsRealmInner*>( JS::GetRealmPrivate( js::GetNonCCWObjectRealm( jsGlobal ) ) );
    assert( pJsRealm );

    return pJsRealm->GetCurrentHeapBytes();
}

uint64_t JsGc::GetTotalHeapUsage() const
{
    return lastTotalHeapSize_;
}

void JsGc::Initialize( JSContext* pJsCtx )
{
    namespace smp_advconf = smp::config::advanced;

    pJsCtx_ = pJsCtx;

    UpdateGcConfig();

    maxHeapSize_ = smp_advconf::gc_max_heap.GetValue();
    heapGrowthRateTrigger_ = smp_advconf::gc_max_heap_growth.GetValue();
    gcSliceTimeBudget_ = smp_advconf::gc_budget.GetValue();
    gcCheckDelay_ = smp_advconf::gc_delay.GetValue();
    allocCountTrigger_ = smp_advconf::gc_max_alloc_increase.GetValue();

    JS_SetGCParameter( pJsCtx_, JSGC_MODE, JSGC_MODE_INCREMENTAL );
    // The following two parameters are not used, since we are doing everything manually.
    // Left here mostly for future-proofing.
    JS_SetGCParameter( pJsCtx_, JSGC_SLICE_TIME_BUDGET, gcSliceTimeBudget_ );
    JS_SetGCParameter( pJsCtx_, JSGC_HIGH_FREQUENCY_TIME_LIMIT, kHighFreqTimeLimitMs );

#ifdef DEBUG
    if ( smp_advconf::zeal.GetValue() )
    {
        JS_SetGCZeal( pJsCtx_,
                      smp_advconf::zeal_level.GetValue(),
                      smp_advconf::zeal_freq.GetValue() );
    }
#endif
}

void JsGc::Finalize()
{
    PerformNormalGc();

    const auto curTime = timeGetTime();

    isHighFrequency_ = false;
    lastGcCheckTime_ = curTime;
    lastGcTime_ = curTime;
    lastTotalHeapSize_ = 0;
    lastTotalAllocCount_ = 0;
    lastGlobalHeapSize_ = 0;
}

bool JsGc::MaybeGc()
{
    assert( pJsCtx_ );
    assert( JS::IsIncrementalGCEnabled( pJsCtx_ ) );

    if ( !IsTimeToGc() )
    {
        return true;
    }

    GcLevel gcLevel = GetRequiredGcLevel();
    if ( GcLevel::None == gcLevel )
    {
        return true;
    }

    PerformGc( gcLevel );
    UpdateGcStats();

    return ( lastTotalHeapSize_ < maxHeapSize_ );
}

bool JsGc::TriggerGc()
{
    isManuallyTriggered_ = true;
    return MaybeGc();
}

void JsGc::UpdateGcConfig()
{
    namespace smp_advconf = smp::config::advanced;

    MEMORYSTATUSEX statex = { 0 };
    statex.dwLength = sizeof( statex );
    BOOL bRet = GlobalMemoryStatusEx( &statex );
    qwr::error::CheckWinApi( !!bRet, "GlobalMemoryStatusEx" );

    if ( !smp_advconf::gc_max_heap.GetValue() )
    { // detect settings automatically
        smp_advconf::gc_max_heap = std::min<uint64_t>( statex.ullTotalPhys / 4, kDefaultHeapMaxMb );
    }
    else if ( smp_advconf::gc_max_heap.GetValue() > statex.ullTotalPhys )
    {
        smp_advconf::gc_max_heap = statex.ullTotalPhys;
    }

    if ( !smp_advconf::gc_max_heap_growth.GetValue() )
    { // detect settings automatically
        smp_advconf::gc_max_heap_growth = std::min<uint64_t>( smp_advconf::gc_max_heap.GetValue() / 8, kDefaultHeapThresholdMb );
    }
    else if ( smp_advconf::gc_max_heap_growth.GetValue() > smp_advconf::gc_max_heap.GetValue() / 2 )
    {
        smp_advconf::gc_max_heap_growth = smp_advconf::gc_max_heap.GetValue() / 2;
    }
}

bool JsGc::IsTimeToGc()
{
    const auto curTime = timeGetTime();
    if ( ( curTime - lastGcCheckTime_ ) < gcCheckDelay_ )
    {
        return false;
    }

    lastGcCheckTime_ = curTime;
    return true;
}

JsGc::GcLevel JsGc::GetRequiredGcLevel()
{
    if ( GcLevel gcLevel = GetGcLevelFromHeapSize();
         gcLevel > GcLevel::None )
    { // heap trigger always has the highest priority
        return gcLevel;
    }
    else if ( JS::IsIncrementalGCInProgress( pJsCtx_ )
              || isManuallyTriggered_
              || GetGcLevelFromAllocCount() > GcLevel::None )
    {                                 // currently alloc trigger can be at most `GcLevel::Incremental`
        isManuallyTriggered_ = false; // reset trigger
        return GcLevel::Incremental;
    }
    else
    {
        return GcLevel::None;
    }
}

JsGc::GcLevel JsGc::GetGcLevelFromHeapSize()
{
    uint64_t curTotalHeapSize = GetCurrentTotalHeapSize();
    if ( !lastTotalHeapSize_
         || lastTotalHeapSize_ > curTotalHeapSize )
    {
        lastTotalHeapSize_ = curTotalHeapSize;
    }

    const uint32_t maxHeapGrowthRate = ( isHighFrequency_ ? kHighFreqHeapGrowthMultiplier * heapGrowthRateTrigger_ : heapGrowthRateTrigger_ );
    if ( curTotalHeapSize <= lastTotalHeapSize_ + maxHeapGrowthRate )
    {
        return GcLevel::None;
    }
    else if ( curTotalHeapSize <= maxHeapSize_ * 0.75 )
    {
        return GcLevel::Incremental;
    }
    else if ( curTotalHeapSize <= maxHeapSize_ * 0.9 )
    {
        return GcLevel::Normal;
    }
    else
    {
        return GcLevel::Full;
    }
}

JsGc::GcLevel JsGc::GetGcLevelFromAllocCount()
{
    uint64_t curTotalAllocCount = GetCurrentTotalAllocCount();
    if ( !lastTotalAllocCount_
         || lastTotalAllocCount_ > curTotalAllocCount )
    {
        lastTotalAllocCount_ = curTotalAllocCount;
    }

    if ( curTotalAllocCount <= lastTotalAllocCount_ + allocCountTrigger_ )
    {
        return GcLevel::None;
    }
    else
    {
        return GcLevel::Incremental;
    }
    // Note: check all method invocations when adding new GcLevel,
    // since currently it's assumed that method returns `GcLevel::Incremental` at most
}

void JsGc::UpdateGcStats()
{
    if ( JS::IsIncrementalGCInProgress( pJsCtx_ ) )
    { // update only after current gc cycle is finished
        return;
    }

    lastGlobalHeapSize_ = JS_GetGCParameter( pJsCtx_, JSGC_BYTES );
    lastTotalHeapSize_ = GetCurrentTotalHeapSize();
    lastTotalAllocCount_ = GetCurrentTotalAllocCount();

    const auto curTime = timeGetTime();
    isHighFrequency_ = ( lastGcTime_
                             ? curTime < ( lastGcTime_ + kHighFreqTimeLimitMs )
                             : false );
    lastGcTime_ = curTime;
}

uint64_t JsGc::GetCurrentTotalHeapSize()
{
    uint64_t curTotalHeapSize = JS_GetGCParameter( pJsCtx_, JSGC_BYTES );

    JS::IterateRealms( pJsCtx_, &curTotalHeapSize, []( JSContext*, void* data, JS::Handle<JS::Realm*> pJsRealm ) {
        auto pCurTotalHeapSize = static_cast<uint64_t*>( data );
        auto pNativeRealm = static_cast<JsRealmInner*>( JS::GetRealmPrivate( pJsRealm ) );
        if ( !pNativeRealm )
        {
            return;
        }
        *pCurTotalHeapSize += pNativeRealm->GetCurrentHeapBytes();
    } );

    return curTotalHeapSize;
}

uint64_t JsGc::GetCurrentTotalAllocCount()
{
    uint64_t curTotalAllocCount = 0;
    JS::IterateRealms( pJsCtx_, &curTotalAllocCount, []( JSContext*, void* data, JS::Handle<JS::Realm*> pJsRealm ) {
        auto pCurTotalAllocCount = static_cast<uint64_t*>( data );
        auto pNativeRealm = static_cast<JsRealmInner*>( JS::GetRealmPrivate( pJsRealm ) );
        if ( !pNativeRealm )
        {
            return;
        }
        *pCurTotalAllocCount += pNativeRealm->GetCurrentAllocCount();
    } );

    return curTotalAllocCount;
}

void JsGc::PerformGc( GcLevel gcLevel )
{
    if ( !JS::IsIncrementalGCInProgress( pJsCtx_ ) )
    {
        PrepareRealmsForGc( gcLevel );
    }

    switch ( gcLevel )
    {
    case mozjs::JsGc::GcLevel::Incremental:
        PerformIncrementalGc();
        break;
    case mozjs::JsGc::GcLevel::Normal:
        PerformNormalGc();
        break;
    case mozjs::JsGc::GcLevel::Full:
        PerformFullGc();
        break;
    default:
        assert( 0 );
        break;
    }

    if ( !JS::IsIncrementalGCInProgress( pJsCtx_ ) )
    {
        NotifyRealmsOnGcEnd();
    }
}

void JsGc::PrepareRealmsForGc( GcLevel gcLevel )
{
    const auto markAllRealms = [&] {
        JS::IterateRealms( pJsCtx_, nullptr, []( JSContext*, void*, JS::Handle<JS::Realm*> pJsRealm ) {
            auto pNativeRealm = static_cast<JsRealmInner*>( JS::GetRealmPrivate( pJsRealm ) );
            if ( !pNativeRealm )
            {
                return;
            }

            pNativeRealm->OnGcStart();
        } );
    };

    switch ( gcLevel )
    {
    case mozjs::JsGc::GcLevel::Incremental:
    {
        struct TriggerData
        {
            uint32_t heapGrowthRateTrigger;
            uint32_t allocCountTrigger;
        };
        TriggerData triggers{
            ( isHighFrequency_ ? kHighFreqHeapGrowthMultiplier * heapGrowthRateTrigger_ : heapGrowthRateTrigger_ ) / 2,
            allocCountTrigger_ / 2
        };

        if ( uint64_t curGlobalHeapSize = JS_GetGCParameter( pJsCtx_, JSGC_BYTES );
             curGlobalHeapSize > ( lastGlobalHeapSize_ + triggers.heapGrowthRateTrigger ) )
        { // mark all, since we don't have any per-realm information about allocated native JS objects
            markAllRealms();
        }
        else
        {
            JS::IterateRealms( pJsCtx_, &triggers, []( JSContext*, void* data, JS::Handle<JS::Realm*> pJsRealm ) {
                const TriggerData& pTriggerData = *reinterpret_cast<const TriggerData*>( data );

                auto pNativeRealm = static_cast<JsRealmInner*>( JS::GetRealmPrivate( pJsRealm ) );
                if ( !pNativeRealm )
                {
                    return;
                }

                const bool hasHeapOvergrowth = pNativeRealm->GetCurrentHeapBytes() > ( pNativeRealm->GetLastHeapBytes() + pTriggerData.heapGrowthRateTrigger );
                const bool hasOveralloc = pNativeRealm->GetCurrentAllocCount() > ( pNativeRealm->GetLastAllocCount() + pTriggerData.allocCountTrigger );
                if ( hasHeapOvergrowth || hasOveralloc || pNativeRealm->IsMarkedForDeletion() )
                {
                    pNativeRealm->OnGcStart();
                }
            } );
        }

        break;
    }
    case mozjs::JsGc::GcLevel::Normal:
    case mozjs::JsGc::GcLevel::Full:
    {
        markAllRealms();
        break;
    }
    default:
        assert( 0 );
        break;
    }
}

void JsGc::PerformIncrementalGc()
{
    const uint32_t sliceBudget = ( isHighFrequency_ ? kHighFreqBudgetMultiplier * gcSliceTimeBudget_ : gcSliceTimeBudget_ );

    if ( !JS::IsIncrementalGCInProgress( pJsCtx_ ) )
    {
        std::vector<JS::Realm*> realms;

        JS::IterateRealms( pJsCtx_, &realms, []( JSContext*, void* data, JS::Handle<JS::Realm*> pJsRealm ) {
            auto pRealms = static_cast<std::vector<JS::Realm*>*>( data );
            auto pNativeRealm = static_cast<JsRealmInner*>( JS::GetRealmPrivate( pJsRealm ) );

            if ( pNativeRealm && pNativeRealm->IsMarkedForGc() )
            {
                pRealms->push_back( pJsRealm );
            }
        } );
        if ( !realms.empty() )
        {
            for ( auto pRealm: realms )
            {
                JS::PrepareZoneForGC( js::GetRealmZone( pRealm ) );
            }
        }
        else
        {
            JS::PrepareForFullGC( pJsCtx_ );
        }

        JS::StartIncrementalGC( pJsCtx_, GC_NORMAL, JS::GCReason::RESERVED4, sliceBudget );
    }
    else
    {
        JS::PrepareForIncrementalGC( pJsCtx_ );
        JS::IncrementalGCSlice( pJsCtx_, JS::GCReason::RESERVED5, sliceBudget );
    }
}

void JsGc::PerformNormalGc()
{
    if ( JS::IsIncrementalGCInProgress( pJsCtx_ ) )
    {
        JS::PrepareForIncrementalGC( pJsCtx_ );
        JS::FinishIncrementalGC( pJsCtx_, JS::GCReason::RESERVED6 );
    }

    JS_GC( pJsCtx_ );
}

void JsGc::PerformFullGc()
{
    if ( JS::IsIncrementalGCInProgress( pJsCtx_ ) )
    {
        JS::PrepareForIncrementalGC( pJsCtx_ );
        JS::FinishIncrementalGC( pJsCtx_, JS::GCReason::RESERVED7 );
    }

    JS_SetGCParameter( pJsCtx_, JSGC_MODE, JSGC_MODE_GLOBAL );
    JS::PrepareForFullGC( pJsCtx_ );
    JS::NonIncrementalGC( pJsCtx_, GC_SHRINK, JS::GCReason::RESERVED8 );
    JS_SetGCParameter( pJsCtx_, JSGC_MODE, JSGC_MODE_INCREMENTAL );
}

void JsGc::NotifyRealmsOnGcEnd()
{
    JS::IterateRealms( pJsCtx_, nullptr, []( JSContext*, void*, JS::Handle<JS::Realm*> pJsRealm ) {
        auto pNativeRealm = static_cast<JsRealmInner*>( JS::GetRealmPrivate( pJsRealm ) );
        if ( !pNativeRealm )
        {
            return;
        }

        if ( pNativeRealm->IsMarkedForGc() )
        {
            pNativeRealm->OnGcDone();
        }
    } );
}

} // namespace mozjs
