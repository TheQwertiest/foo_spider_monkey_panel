#include <stdafx.h>
#include "js_engine.h"

#include <js_engine/js_container.h>
#include <js_engine/js_compartment_inner.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/scope_helper.h>

#include <js_panel_window.h>
#include <adv_config.h>
#include <user_message.h>
#include <heartbeat_window.h>

#include <js/Initialization.h>

namespace
{
const uint32_t kHeartbeatRate = 73; ///< In ms
const uint32_t kJobsMaxBudget = 500; ///< In ms
}

namespace mozjs
{

JsEngine::JsEngine()
{
    JS_Init();
}

JsEngine::~JsEngine()
{// Can't clean up here, since mozjs.dll might be already unloaded
    assert( !isInitialized_ );
}

JsEngine& JsEngine::GetInstance()
{
    static JsEngine jsEnv;
    return jsEnv;
}

bool JsEngine::Initialize()
{
    namespace smp_advconf = smp::config::advanced;

    if ( isInitialized_ )
    {
        return true;
    }

    MEMORYSTATUSEX statex = { 0 };
    statex.dwLength = sizeof( statex );
    if ( !GlobalMemoryStatusEx( &statex ) )
    {
        // TODO: add report
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

    maxHeapSize_ = static_cast<uint32_t>(smp_advconf::g_var_max_heap.get());
    heapGrowthRateTrigger_ = static_cast<uint32_t>(smp_advconf::g_var_max_heap_growth.get());
    gcSliceTimeBudget_ = static_cast<uint32_t>(smp_advconf::g_var_gc_budget.get());
    gcCheckDelay_ = static_cast<uint32_t>(smp_advconf::g_var_gc_delay.get());
    allocCountTrigger_ = static_cast<uint32_t>( smp_advconf::g_var_max_alloc_increase.get() );

    JSContext* pJsCtx = JS_NewContext( maxHeapSize_ );
    if ( !pJsCtx )
    {//reports
        return false;
    }

    scope::unique_ptr<JSContext> autoJsCtx( pJsCtx, []( auto pCtx )
    {
        JS_DestroyContext( pCtx );
    } );

    if ( !js::UseInternalJobQueues( pJsCtx ) )
    { //reports
        return false;
    }

    // TODO: JS::SetWarningReporter( pJsCtx_ )

    if ( !JS::InitSelfHostedCode( pJsCtx ) )
    {//reports
        return false;
    }

    JS_SetGCParameter( pJsCtx, JSGC_MODE, JSGC_MODE_INCREMENTAL );

#ifdef DEBUG
    if ( smp_advconf::g_var_gc_zeal.get() )
    {
        JS_SetGCZeal( pJsCtx,
                      static_cast<uint8_t>(smp_advconf::g_var_gc_zeal_level.get()), 
                      static_cast<uint32_t>(smp_advconf::g_var_gc_zeal_freq.get()) );
    }
#endif

    pJsCtx_ = autoJsCtx.release();

    if ( !StartHeartbeatThread() )
    {// TODO: error report
        return false;
    }

    isInitialized_ = true;
    return true;
}

void JsEngine::Finalize()
{
    if ( pJsCtx_ )
    {
        StopHeartbeatThread();

        for ( auto&[hWnd, jsContainer] : registeredContainers_ )
        {
            jsContainer.get().Finalize();
        }

        JS_DestroyContext( pJsCtx_ );
        pJsCtx_ = nullptr;
    }

    if ( shouldShutdown_ )
    {
        JS_ShutDown();
    }

    isInitialized_ = false;
}

void JsEngine::OnHeartbeat()
{
    if ( !isInitialized_ )
    {
        return;
    }

    JSAutoRequest ar( pJsCtx_ );

    MaybeRunJobs();
    MaybeGc();
}

bool JsEngine::StartHeartbeatThread()
{
    if ( !heartbeatWindow_ )
    {
        heartbeatWindow_ = smp::HeartbeatWindow::Create();
        if ( !heartbeatWindow_ )
        {
            return false;
        }
    }

    shouldStopHeartbeatThread_ = false;
    heartbeatThread_ = std::thread( [parent = this] {
        while ( !parent->shouldStopHeartbeatThread_ )
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds( kHeartbeatRate ) 
            );

            PostMessage( parent->heartbeatWindow_->GetHwnd(), UWM_HEARTBEAT, 0, 0 );
        }
    } );

    return true;
}

void JsEngine::StopHeartbeatThread()
{
    if ( heartbeatThread_.joinable() )
    {
        shouldStopHeartbeatThread_ = true;
        heartbeatThread_.join();
    }
}

void JsEngine::PrepareForExit()
{
    shouldShutdown_ = true;
}

bool JsEngine::RegisterPanel( js_panel_window& panel, JsContainer& jsContainer )
{
    if ( !registeredContainers_.size() && !Initialize() )
    {
        return false;
    }

    if ( !jsContainer.Prepare( pJsCtx_, panel ) )
    {
        return false;
    }

    registeredContainers_.insert_or_assign( panel.GetHWND(), jsContainer );

    return true;
}

void JsEngine::UnregisterPanel( js_panel_window& parentPanel )
{
    auto elem = registeredContainers_.find( parentPanel.GetHWND() );
    if ( elem != registeredContainers_.end() )
    {
        elem->second.get().Finalize();
        registeredContainers_.erase( elem );
    }

    if ( !registeredContainers_.size() )
    {
        Finalize();
    }
}

void JsEngine::MaybeGc()
{// TODO: cleanup
    assert( JS::IsIncrementalGCEnabled( pJsCtx_ ) );

    if ( timeGetTime() - lastGcCheckTime_ < gcCheckDelay_ )
    {
        return;
    }
    lastGcCheckTime_ = timeGetTime();

    uint64_t curTotalHeapSize = GetCurrentTotalHeapSize();
    if ( !lastTotalHeapSize_ )
    {
        lastTotalHeapSize_ = curTotalHeapSize;
    }

    uint64_t curTotalAllocCount = GetCurrentTotalAllocCount();
    if ( !lastTotalAllocCount_ )
    {
        lastTotalAllocCount_ = curTotalAllocCount;
    }

    if ( !JS::IsIncrementalGCInProgress( pJsCtx_ ) 
         && ( curTotalHeapSize <= lastTotalHeapSize_ + heapGrowthRateTrigger_ )
         && ( curTotalAllocCount <= lastTotalAllocCount_ + allocCountTrigger_ ) )
    {
        if ( lastTotalHeapSize_ > curTotalHeapSize )
        {
            lastTotalHeapSize_ = curTotalHeapSize;
        }
        if ( lastTotalAllocCount_ > curTotalAllocCount )
        {
            lastTotalAllocCount_ = curTotalAllocCount;
        }
        return;
    }

    GcLevel gcLevel;
    if ( curTotalHeapSize <= maxHeapSize_ / 2 )
    {// alloc trigger usually goes here as well
        gcLevel = GcLevel::Incremental;
    }
    else if ( curTotalHeapSize <= maxHeapSize_ * 0.75 )
    {
        gcLevel = GcLevel::Normal;
    }
    else
    {
        gcLevel = GcLevel::Full;
    }

    PerformGc( gcLevel );

    lastTotalHeapSize_ = curTotalHeapSize;
    lastTotalAllocCount_ = curTotalAllocCount;

    if ( curTotalHeapSize >= maxHeapSize_ )
    {
        FB2K_console_formatter() << "Out of memory error: " SMP_NAME_WITH_VERSION;
        for ( auto& [hWnd, jsContainer] : registeredContainers_ )
        {
            jsContainer.get().Fail( "Out of memory" );
        }
    }
}

uint64_t JsEngine::GetCurrentTotalHeapSize()
{
    uint64_t curTotalHeapSize = JS_GetGCParameter( pJsCtx_, JSGC_BYTES );
    JS_IterateCompartments( pJsCtx_, &curTotalHeapSize, []( JSContext* cx, void* data, JSCompartment* pJsCompartment )
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

uint64_t JsEngine::GetCurrentTotalAllocCount()
{
    uint64_t curTotalAllocCount = JS_GetGCParameter( pJsCtx_, JSGC_BYTES );
    JS_IterateCompartments( pJsCtx_, &curTotalAllocCount, []( JSContext* cx, void* data, JSCompartment* pJsCompartment ) {
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

void JsEngine::PerformGc( GcLevel gcLevel )
{
    if ( !JS::IsIncrementalGCInProgress( pJsCtx_ ) )
    {
        PrepareCompartmentsForGc( gcLevel );
    }

    switch ( gcLevel )
    {
    case mozjs::JsEngine::GcLevel::Incremental:
        PerformIncrementalGc();
        break;
    case mozjs::JsEngine::GcLevel::Normal:
        PerformNormalGc();
        break;
    case mozjs::JsEngine::GcLevel::Full:
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

void JsEngine::PerformIncrementalGc()
{
    if ( !JS::IsIncrementalGCInProgress( pJsCtx_ ) )
    {
        std::vector<JSCompartment*> compartments;
        JS_IterateCompartments( pJsCtx_, &compartments, []( JSContext* cx, void* data, JSCompartment* pJsCompartment )
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

void JsEngine::PerformNormalGc()
{
    if ( JS::IsIncrementalGCInProgress( pJsCtx_ ) )
    {
        JS::PrepareForIncrementalGC( pJsCtx_ );
        JS::FinishIncrementalGC( pJsCtx_, JS::gcreason::RESERVED3 );
    }

    JS_GC( pJsCtx_ );
}

void JsEngine::PerformFullGc()
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

void JsEngine::PrepareCompartmentsForGc( GcLevel gcLevel )
{
    switch ( gcLevel )
    {
    case mozjs::JsEngine::GcLevel::Incremental:
    {
        using TriggerData = struct  
        {
            uint32_t heapGrowthRateTrigger;
            uint32_t allocCountTrigger;
        };
        TriggerData triggers{ heapGrowthRateTrigger_, allocCountTrigger_ };
        JS_IterateCompartments( pJsCtx_, &triggers, []( JSContext* cx, void* data, JSCompartment* pJsCompartment )
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
    case mozjs::JsEngine::GcLevel::Normal:
    case mozjs::JsEngine::GcLevel::Full:
    {
        JS_IterateCompartments( pJsCtx_, nullptr, []( JSContext* cx, void* data, JSCompartment* pJsCompartment )
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

void JsEngine::NotifyCompartmentsOnGcEnd()
{
    JS_IterateCompartments( pJsCtx_, nullptr, []( JSContext* cx, void* data, JSCompartment* pJsCompartment )
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

void JsEngine::MaybeRunJobs()
{
    if ( areJobsInProgress_ )
    {
        if ( timeGetTime() - jobsStartTime_ >= kJobsMaxBudget )
        {
            js::StopDrainingJobQueue( pJsCtx_ );
        }
        return;
    }

    jobsStartTime_ = timeGetTime();
    areJobsInProgress_ = true; 
    js::RunJobs( pJsCtx_ );
    areJobsInProgress_ = false; 
}

}
