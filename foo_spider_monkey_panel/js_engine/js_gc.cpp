#include <stdafx.h>
#include "js_gc.h"

#include <js_engine/js_container.h>
#include <js_engine/js_compartment_inner.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/scope_helper.h>

#include <adv_config.h>


namespace mozjs
{

uint32_t JsGc::GetMaxHeap()
{
    namespace smp_advconf = smp::config::advanced;

    if ( !UpdateGcConfig() )
    {// TODO: report
        assert( 0 );
        return 0;
    }

    return static_cast<uint32_t>( smp_advconf::g_var_max_heap.get() );
}

bool JsGc::Initialize(JSContext* pJsCtx)
{
    namespace smp_advconf = smp::config::advanced;

    pJsCtx_ = pJsCtx;

    UpdateGcConfig();

    maxHeapSize_ = static_cast<uint32_t>(smp_advconf::g_var_max_heap.get());
    heapGrowthRateTrigger_ = static_cast<uint32_t>(smp_advconf::g_var_max_heap_growth.get());
    gcSliceTimeBudget_ = static_cast<uint32_t>(smp_advconf::g_var_gc_budget.get());
    gcCheckDelay_ = static_cast<uint32_t>(smp_advconf::g_var_gc_delay.get());
    allocCountTrigger_ = static_cast<uint32_t>( smp_advconf::g_var_max_alloc_increase.get() );

    JS_SetGCParameter( pJsCtx_, JSGC_MODE, JSGC_MODE_INCREMENTAL );

#ifdef DEBUG
    if ( smp_advconf::g_var_gc_zeal.get() )
    {
        JS_SetGCZeal( pJsCtx_,
                      static_cast<uint8_t>(smp_advconf::g_var_gc_zeal_level.get()), 
                      static_cast<uint32_t>(smp_advconf::g_var_gc_zeal_freq.get()) );
    }
#endif

    return true;
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

bool JsGc::UpdateGcConfig()
{
    namespace smp_advconf = smp::config::advanced;

    MEMORYSTATUSEX statex = { 0 };
    statex.dwLength = sizeof( statex );
    if ( !GlobalMemoryStatusEx( &statex ) )
    {// TODO: add report        
        return false;
    }

    if ( smp_advconf::g_var_max_heap.get() > statex.ullTotalPhys / 4 )
    {
        smp_advconf::g_var_max_heap.set( statex.ullTotalPhys / 4 );
    }

    if ( smp_advconf::g_var_max_heap_growth.get() > smp_advconf::g_var_max_heap.get() / 8 )
    {
        smp_advconf::g_var_max_heap_growth.set( smp_advconf::g_var_max_heap.get() / 8 );
    }

    return true;
}

bool JsGc::IsTimeToGc()
{
    if ( timeGetTime() - lastGcCheckTime_ < gcCheckDelay_ )
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

    if ( curTotalHeapSize <= lastTotalHeapSize_ + heapGrowthRateTrigger_ )
    {
        return GcLevel::None;
    }
    else if ( curTotalHeapSize <= maxHeapSize_ / 2 )
    {
        return GcLevel::Incremental;
    }
    else if ( curTotalHeapSize <= maxHeapSize_ * 0.75 )
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

void JsGc::UpdateGcStats()
{
    if ( !JS::IsIncrementalGCInProgress( pJsCtx_ ) )
    { // update only after current gc cycle is finished
        lastTotalHeapSize_ = GetCurrentTotalHeapSize();
        lastTotalAllocCount_ = GetCurrentTotalAllocCount();
    }
}

uint64_t JsGc::GetCurrentTotalHeapSize()
{
    uint64_t curTotalHeapSize = JS_GetGCParameter( pJsCtx_, JSGC_BYTES );
    JS_IterateCompartments( pJsCtx_, &curTotalHeapSize, []( JSContext*, void* data, JSCompartment* pJsCompartment )
    {
        auto pCurTotalHeapSize = static_cast<uint64_t*>(data);
        auto pNativeCompartment = static_cast<JsCompartmentInner*>(JS_GetCompartmentPrivate( pJsCompartment ));
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
    if ( !JS::IsIncrementalGCInProgress( pJsCtx_ ) )
    {
        std::vector<JSCompartment*> compartments;
        JS_IterateCompartments( pJsCtx_, &compartments, []( JSContext*, void* data, JSCompartment* pJsCompartment )
        {
            auto pJsCompartments = static_cast<std::vector<JSCompartment*>*>(data);
            auto pNativeCompartment = static_cast<JsCompartmentInner*>(JS_GetCompartmentPrivate( pJsCompartment ));
            if ( !pNativeCompartment )
            {
                return;
            }

            if ( pNativeCompartment->IsMarkedForGc() )
            {
                pJsCompartments->push_back( pJsCompartment );
            }
        } );
        if ( compartments.size() )
        {
            for ( auto pCompartment : compartments )
            {
                JS::PrepareZoneForGC( js::GetCompartmentZone( pCompartment ) );
            }
        }
        else
        {
            JS::PrepareForFullGC( pJsCtx_ );
        }

        JS::StartIncrementalGC( pJsCtx_, GC_NORMAL, JS::gcreason::RESERVED1, gcSliceTimeBudget_ );
    }
    else
    {
        JS::PrepareForIncrementalGC( pJsCtx_ );
        JS::IncrementalGCSlice( pJsCtx_, JS::gcreason::RESERVED2, gcSliceTimeBudget_ );
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
        using TriggerData = struct  
        {
            uint32_t heapGrowthRateTrigger;
            uint32_t allocCountTrigger;
        };
        TriggerData triggers{ heapGrowthRateTrigger_, allocCountTrigger_ };
        JS_IterateCompartments( pJsCtx_, &triggers, []( JSContext*, void* data, JSCompartment* pJsCompartment )
        {
            TriggerData* pTriggerData = reinterpret_cast<TriggerData*>( data );

            auto pNativeCompartment = static_cast<JsCompartmentInner*>(JS_GetCompartmentPrivate( pJsCompartment ));
            if ( !pNativeCompartment )
            {
                return;
            }

            bool hasHeapOvergrowth = pNativeCompartment->GetCurrentHeapBytes() > ( pNativeCompartment->GetLastHeapBytes() + pTriggerData->heapGrowthRateTrigger / 2 );
            bool hasOveralloc = pNativeCompartment->GetCurrentAllocCount() > ( pNativeCompartment->GetLastAllocCount() + pTriggerData->allocCountTrigger / 2 );
            if ( hasHeapOvergrowth || hasOveralloc ||pNativeCompartment->IsMarkedForDeletion() )
            {
                pNativeCompartment->OnGcStart();
            }
        } );
        break;
    }
    case mozjs::JsGc::GcLevel::Normal:
    case mozjs::JsGc::GcLevel::Full:
    {
        JS_IterateCompartments( pJsCtx_, nullptr, []( JSContext*, void*, JSCompartment* pJsCompartment )
        {
            auto pNativeCompartment = static_cast<JsCompartmentInner*>(JS_GetCompartmentPrivate( pJsCompartment ));
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
    JS_IterateCompartments( pJsCtx_, nullptr, []( JSContext*, void*, JSCompartment* pJsCompartment )
    {
        auto pNativeCompartment = static_cast<JsCompartmentInner*>(JS_GetCompartmentPrivate( pJsCompartment ));
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

}
