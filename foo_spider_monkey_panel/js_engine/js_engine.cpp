#include <stdafx.h>
#include "js_engine.h"

#include <js_engine/js_container.h>
#include <js_engine/js_compartment_inner.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/scope_helper.h>

#include <js_panel_window.h>

#include <js/Initialization.h>


namespace
{
// TODO: Fine tune heap settings 

/// @brief Max total size of both GC heap and SMP heap
const uint64_t k_MaxHeapSize = 1024L * 1024 * 1024;
/// @brief Max heap size difference before triggering GC
const uint64_t k_HeapGrowthRateTrigger = 50L * 1024 * 1024;
/// @brief Time in ms that an incremental slice is allowed to run
const uint32_t k_GcSliceTimeBudget = 30;
/// @brief Delay in ms between GC checks
const uint32_t k_GcCheckDelay = 50;

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
    if ( isInitialized_ )
    {
        return true;
    }

    JSContext* pJsCtx = JS_NewContext( k_MaxHeapSize );
    if (!pJsCtx)
    {
        return false;
    }

    scope::unique_ptr<JSContext> autoJsCtx( pJsCtx, []( auto pCtx )
    {
        JS_DestroyContext( pCtx );
    } );

    // TODO: JS::SetWarningReporter( pJsCtx_ )

    if (!JS::InitSelfHostedCode( pJsCtx ))
    {
        return false;
    }

    JS_SetGCParameter( pJsCtx, JSGC_MODE, JSGC_MODE_INCREMENTAL );

#ifdef DEBUG
    //JS_SetGCZeal( pJsCtx, 2, 200 );
#endif

    pJsCtx_ = autoJsCtx.release();
    isInitialized_ = true;
    return true;
}

void JsEngine::Finalize()
{
    if (pJsCtx_)
    {
        for (auto& [hWnd, jsContainer] : registeredContainers_ )
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

    registeredContainers_.insert_or_assign( panel.GetHWND(), jsContainer);
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
{
    assert( JS::IsIncrementalGCEnabled( pJsCtx_ ) );

    if ( timeGetTime() - lastGcCheckTime_ < k_GcCheckDelay )
    {
        return;
    }
    lastGcCheckTime_ = timeGetTime();

    uint64_t curTotalHeapSize = GetCurrentTotalHeapSize();

    if ( !JS::IsIncrementalGCInProgress( pJsCtx_ )
         && (curTotalHeapSize <= lastTotalHeapSize_ + k_HeapGrowthRateTrigger) )
    {
        if ( !lastTotalHeapSize_ || lastTotalHeapSize_ > curTotalHeapSize )
        {
            lastTotalHeapSize_ = curTotalHeapSize;            
        }
        return;
    }

    GcLevel gcLevel;
    if ( curTotalHeapSize <= k_MaxHeapSize / 2 )
    {
        gcLevel = GcLevel::Incremental;
    }
    else if ( curTotalHeapSize <= k_MaxHeapSize * 0.75 )
    {
        gcLevel = GcLevel::Normal;
    }
    else
    {
        gcLevel = GcLevel::Full;
    }

    PerformGc( gcLevel );

    lastTotalHeapSize_ = curTotalHeapSize;
}

uint64_t JsEngine::GetCurrentTotalHeapSize()
{
    uint64_t curTotalHeapSize = JS_GetGCParameter( pJsCtx_, JSGC_BYTES );
    JS_IterateCompartments( pJsCtx_, &curTotalHeapSize, []( JSContext* cx, void* data, JSCompartment* pJsCompartment )
    {
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
            auto pJsCompartments = static_cast<std::vector<JSCompartment*>*>( data );
            auto pNativeCompartment = static_cast<JsCompartmentInner*>( JS_GetCompartmentPrivate( pJsCompartment ) );
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

        JS::StartIncrementalGC( pJsCtx_, GC_NORMAL, JS::gcreason::RESERVED1, k_GcSliceTimeBudget );
    }
    else
    {
        JS::PrepareForIncrementalGC( pJsCtx_ );
        JS::IncrementalGCSlice( pJsCtx_, JS::gcreason::RESERVED2, k_GcSliceTimeBudget );
    }
}

void JsEngine::PerformNormalGc()
{
    if ( JS::IsIncrementalGCInProgress( pJsCtx_ ) )
    {
        JS::PrepareForIncrementalGC( pJsCtx_ );
        JS::FinishIncrementalGC( pJsCtx_, JS::gcreason::RESERVED3 );
    }
    else
    {
        JS_GC( pJsCtx_ );
    }
}

void JsEngine::PerformFullGc()
{
    if ( JS::IsIncrementalGCInProgress( pJsCtx_ ) )
    {
        JS::PrepareForIncrementalGC( pJsCtx_ );
        JS::FinishIncrementalGC( pJsCtx_, JS::gcreason::RESERVED4 );
    }
    else
    {
        JS_SetGCParameter( pJsCtx_, JSGC_MODE, JSGC_MODE_GLOBAL );
        JS::PrepareForFullGC( pJsCtx_ );
        JS::GCForReason( pJsCtx_, GC_SHRINK, JS::gcreason::RESERVED5 );
        JS_SetGCParameter( pJsCtx_, JSGC_MODE, JSGC_MODE_INCREMENTAL );
    }
}

void JsEngine::PrepareCompartmentsForGc( GcLevel gcLevel )
{
    switch ( gcLevel )
    {
    case mozjs::JsEngine::GcLevel::Incremental:
    {
        JS_IterateCompartments( pJsCtx_, nullptr, []( JSContext* cx, void* data, JSCompartment* pJsCompartment )
        {
            auto pNativeCompartment = static_cast<JsCompartmentInner*>( JS_GetCompartmentPrivate( pJsCompartment ) );
            if ( !pNativeCompartment )
            {
                return;
            }

            bool hasHeapOverGrowth = pNativeCompartment->GetCurrentHeapBytes() > ( pNativeCompartment->GetLastHeapBytes() + k_HeapGrowthRateTrigger / 2 );
            if ( hasHeapOverGrowth || pNativeCompartment->IsMarkedForDeletion() )
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

void JsEngine::NotifyCompartmentsOnGcEnd()
{
    JS_IterateCompartments( pJsCtx_, nullptr, []( JSContext* cx, void* data, JSCompartment* pJsCompartment )
    {
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

}
