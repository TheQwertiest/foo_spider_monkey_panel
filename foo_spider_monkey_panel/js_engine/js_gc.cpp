#include <stdafx.h>
#include "js_gc.h"

#include <js_engine/js_container.h>
#include <js_engine/js_compartment_inner.h>
#include <js_utils/js_error_helper.h>
#include <utils/scope_helpers.h>
#include <utils/winapi_error_helpers.h>

#include <adv_config.h>

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

    return static_cast<uint32_t>( smp_advconf::g_var_max_heap.get() );
}

uint64_t JsGc::GetTotalHeapUsageForGlobal( JSContext* cx, JS::HandleObject jsGlobal )
{
    assert( jsGlobal );

    auto pJsCompartment = static_cast<JsCompartmentInner*>( JS_GetCompartmentPrivate( js::GetObjectCompartment( jsGlobal ) ) );
    assert( pJsCompartment );

    return pJsCompartment->GetCurrentHeapBytes();
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

    maxHeapSize_ = static_cast<uint32_t>( smp_advconf::g_var_max_heap.get() );
    heapGrowthRateTrigger_ = static_cast<uint32_t>( smp_advconf::g_var_max_heap_growth.get() );
    gcSliceTimeBudget_ = static_cast<uint32_t>( smp_advconf::g_var_gc_budget.get() );
    gcCheckDelay_ = static_cast<uint32_t>( smp_advconf::g_var_gc_delay.get() );
    allocCountTrigger_ = static_cast<uint32_t>( smp_advconf::g_var_max_alloc_increase.get() );

    JS_SetGCParameter( pJsCtx_, JSGC_MODE, JSGC_MODE_INCREMENTAL );

#ifdef DEBUG
    if ( smp_advconf::g_var_gc_zeal.get() )
    {
        JS_SetGCZeal( pJsCtx_,
                      static_cast<uint8_t>( smp_advconf::g_var_gc_zeal_level.get() ),
                      static_cast<uint32_t>( smp_advconf::g_var_gc_zeal_freq.get() ) );
    }
#endif
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

    UpdateGcTime();
    PerformGc( gcLevel );
    UpdateGcStats();

    return ( lastTotalHeapSize_ < maxHeapSize_ );
}

void JsGc::UpdateGcConfig()
{
    namespace smp_advconf = smp::config::advanced;

    MEMORYSTATUSEX statex = { 0 };
    statex.dwLength = sizeof( statex );
    BOOL bRet = GlobalMemoryStatusEx( &statex );
    smp::error::CheckWinApi( !!bRet, "GlobalMemoryStatusEx" );

    if ( !smp_advconf::g_var_max_heap.get() )
    { // detect
        smp_advconf::g_var_max_heap.set( std::min<uint64_t>( statex.ullTotalPhys / 4, kDefaultHeapMaxMb ) );
    }
    else if ( smp_advconf::g_var_max_heap.get() > statex.ullTotalPhys )
    {
        smp_advconf::g_var_max_heap.set( statex.ullTotalPhys );
    }

    if ( !smp_advconf::g_var_max_heap_growth.get() )
    { // detect
        smp_advconf::g_var_max_heap_growth.set( std::min<uint64_t>( smp_advconf::g_var_max_heap.get() / 8, kDefaultHeapThresholdMb ) );
    }
    else if ( smp_advconf::g_var_max_heap_growth.get() > smp_advconf::g_var_max_heap.get() / 2 )
    {
        smp_advconf::g_var_max_heap_growth.set( smp_advconf::g_var_max_heap.get() / 2 );
    }
}

bool JsGc::IsTimeToGc()
{
    auto timeDiff = timeGetTime() - lastGcCheckTime_;
    if ( timeDiff < gcCheckDelay_ )
    {
        return false;
    }

    lastGcCheckTime_ = timeGetTime();
    return true;
}

JsGc::GcLevel JsGc::GetRequiredGcLevel()
{
    GcLevel gcLevel = GetGcLevelFromHeapSize();
    if ( JS::IsIncrementalGCInProgress( pJsCtx_ )
         || gcLevel > GcLevel::None )
    {
        return std::max( GcLevel::Incremental, gcLevel );
    }

    return GetGcLevelFromAllocCount();
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
}

void JsGc::UpdateGcTime()
{
    isHighFrequency_ = ( timeGetTime() < ( lastGcTime_ + kHighFreqTimeLimitMs ) );
    lastGcTime_ = timeGetTime();
}

void JsGc::UpdateGcStats()
{
    if ( !JS::IsIncrementalGCInProgress( pJsCtx_ ) )
    { // update only after current gc cycle is finished
        lastGlobalHeapSize_ = JS_GetGCParameter( pJsCtx_, JSGC_BYTES );
        lastTotalHeapSize_ = GetCurrentTotalHeapSize();
        lastTotalAllocCount_ = GetCurrentTotalAllocCount();
    }
}

uint64_t JsGc::GetCurrentTotalHeapSize()
{
    uint64_t curTotalHeapSize = JS_GetGCParameter( pJsCtx_, JSGC_BYTES );

    JS_IterateCompartments( pJsCtx_, &curTotalHeapSize, []( JSContext*, void* data, JSCompartment* pJsCompartment ) {
        auto pCurTotalHeapSize = static_cast<uint64_t*>( data );
        auto pNativeCompartment = static_cast<JsCompartmentInner*>( JS_GetCompartmentPrivate( pJsCompartment ) );
        if ( !pNativeCompartment )
        {
            return;
        }
        *pCurTotalHeapSize += pNativeCompartment->GetCurrentHeapBytes();
    } );

    return curTotalHeapSize;
}

uint64_t JsGc::GetCurrentTotalAllocCount()
{
    uint64_t curTotalAllocCount = 0;
    JS_IterateCompartments( pJsCtx_, &curTotalAllocCount, []( JSContext*, void* data, JSCompartment* pJsCompartment ) {
        auto pCurTotalAllocCount = static_cast<uint64_t*>( data );
        auto pNativeCompartment = static_cast<JsCompartmentInner*>( JS_GetCompartmentPrivate( pJsCompartment ) );
        if ( !pNativeCompartment )
        {
            return;
        }
        *pCurTotalAllocCount += pNativeCompartment->GetCurrentAllocCount();
    } );

    return curTotalAllocCount;
}

void JsGc::PerformGc( GcLevel gcLevel )
{
    if ( !JS::IsIncrementalGCInProgress( pJsCtx_ ) )
    {
        PrepareCompartmentsForGc( gcLevel );
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
        NotifyCompartmentsOnGcEnd();
    }
}

void JsGc::PerformIncrementalGc()
{
    const uint32_t sliceBudget = ( isHighFrequency_ ? kHighFreqBudgetMultiplier * gcSliceTimeBudget_ : gcSliceTimeBudget_ );

    if ( !JS::IsIncrementalGCInProgress( pJsCtx_ ) )
    {
        using ItData = struct
        {
            std::vector<JSCompartment*> compartments;
            bool needToCleanGlobalHeap;
        };

        ItData itData;
        itData.needToCleanGlobalHeap = needToCleanGlobalHeap_;
        
        JS_IterateCompartments( pJsCtx_, &itData, []( JSContext*, void* data, JSCompartment* pJsCompartment ) {
            auto itData = static_cast<ItData*>( data );
            auto pNativeCompartment = static_cast<JsCompartmentInner*>( JS_GetCompartmentPrivate( pJsCompartment ) );

            if ( ( !pNativeCompartment && itData->needToCleanGlobalHeap )
                 || ( pNativeCompartment && pNativeCompartment->IsMarkedForGc() ) )
            {
                itData->compartments.push_back( pJsCompartment );
            }
        } );
        if ( itData.compartments.size() )
        {
            for ( auto pCompartment: itData.compartments )
            {
                JS::PrepareZoneForGC( js::GetCompartmentZone( pCompartment ) );
            }
        }
        else
        {
            JS::PrepareForFullGC( pJsCtx_ );
        }

        JS::StartIncrementalGC( pJsCtx_, GC_NORMAL, JS::gcreason::RESERVED1, sliceBudget );
    }
    else
    {
        JS::PrepareForIncrementalGC( pJsCtx_ );
        JS::IncrementalGCSlice( pJsCtx_, JS::gcreason::RESERVED2, sliceBudget );
    }
}

void JsGc::PerformNormalGc()
{
    if ( JS::IsIncrementalGCInProgress( pJsCtx_ ) )
    {
        JS::PrepareForIncrementalGC( pJsCtx_ );
        JS::FinishIncrementalGC( pJsCtx_, JS::gcreason::RESERVED3 );
    }

    JS_GC( pJsCtx_ );
}

void JsGc::PerformFullGc()
{
    if ( JS::IsIncrementalGCInProgress( pJsCtx_ ) )
    {
        JS::PrepareForIncrementalGC( pJsCtx_ );
        JS::FinishIncrementalGC( pJsCtx_, JS::gcreason::RESERVED4 );
    }

    JS_SetGCParameter( pJsCtx_, JSGC_MODE, JSGC_MODE_GLOBAL );
    JS::PrepareForFullGC( pJsCtx_ );
    JS::GCForReason( pJsCtx_, GC_SHRINK, JS::gcreason::RESERVED5 );
    JS_SetGCParameter( pJsCtx_, JSGC_MODE, JSGC_MODE_INCREMENTAL );
}

void JsGc::PrepareCompartmentsForGc( GcLevel gcLevel )
{
    switch ( gcLevel )
    {
    case mozjs::JsGc::GcLevel::Incremental:
    {
        struct TriggerData
        {
            uint32_t heapGrowthRateTrigger;
            uint32_t allocCountTrigger;
        };
        const uint32_t maxHeapGrowthRate = ( isHighFrequency_ ? kHighFreqHeapGrowthMultiplier * heapGrowthRateTrigger_ : heapGrowthRateTrigger_ );
        TriggerData triggers{ maxHeapGrowthRate, allocCountTrigger_ };

        if ( uint64_t curGlobalHeapSize = JS_GetGCParameter( pJsCtx_, JSGC_BYTES );
             curGlobalHeapSize > ( lastGlobalHeapSize_ + triggers.heapGrowthRateTrigger ) )
        {
            needToCleanGlobalHeap_ = true;
        }

        JS_IterateCompartments( pJsCtx_, &triggers, []( JSContext*, void* data, JSCompartment* pJsCompartment ) {
            const TriggerData& pTriggerData = *reinterpret_cast<const TriggerData*>( data );

            auto pNativeCompartment = static_cast<JsCompartmentInner*>( JS_GetCompartmentPrivate( pJsCompartment ) );
            if ( !pNativeCompartment )
            {
                return;
            }

            const bool hasHeapOvergrowth = pNativeCompartment->GetCurrentHeapBytes() > ( pNativeCompartment->GetLastHeapBytes() + pTriggerData.heapGrowthRateTrigger / 2 );
            const bool hasOveralloc = pNativeCompartment->GetCurrentAllocCount() > ( pNativeCompartment->GetLastAllocCount() + pTriggerData.allocCountTrigger / 2 );
            if ( hasHeapOvergrowth || hasOveralloc || pNativeCompartment->IsMarkedForDeletion() )
            {
                pNativeCompartment->OnGcStart();
            }
        } );
        break;
    }
    case mozjs::JsGc::GcLevel::Normal:
    case mozjs::JsGc::GcLevel::Full:
    {
        JS_IterateCompartments( pJsCtx_, nullptr, []( JSContext*, void*, JSCompartment* pJsCompartment ) {
            auto pNativeCompartment = static_cast<JsCompartmentInner*>( JS_GetCompartmentPrivate( pJsCompartment ) );
            if ( !pNativeCompartment )
            {
                return;
            }

            pNativeCompartment->OnGcStart();
        } );
        break;
    }
    default:
        assert( 0 );
        break;
    }
}

void JsGc::NotifyCompartmentsOnGcEnd()
{
    JS_IterateCompartments( pJsCtx_, nullptr, []( JSContext*, void*, JSCompartment* pJsCompartment ) {
        auto pNativeCompartment = static_cast<JsCompartmentInner*>( JS_GetCompartmentPrivate( pJsCompartment ) );
        if ( !pNativeCompartment )
        {
            return;
        }

        if ( pNativeCompartment->IsMarkedForGc() )
        {
            pNativeCompartment->OnGcDone();
        }
    } );
}

} // namespace mozjs
