#include <stdafx.h>
#include "js_engine.h"

#include <js_engine/js_container.h>
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

void JsEngine::OnHeapAllocate( uint32_t size )
{
    std::scoped_lock sl( gcLock_ );
    curNativeHeapSize_ += size;
}

void JsEngine::OnHeapDeallocate( uint32_t size )
{
    std::scoped_lock sl( gcLock_ );
    if ( size > curNativeHeapSize_ )
    {
        curNativeHeapSize_ = 0;
    }
    else
    {
        curNativeHeapSize_ -= size;
    }    
}

void JsEngine::MaybeIncrementalGC( JSContext* cx )
{
    assert( JS::IsIncrementalGCEnabled( cx ) );

    if ( timeGetTime() - lastGcCheckTime_ < k_GcCheckDelay )
    {
        return;
    }
    lastGcCheckTime_ = timeGetTime();

    uint64_t curTotalHeapSize = JS_GetGCParameter( cx, JSGC_BYTES );
    {
        std::scoped_lock sl( gcLock_ );
        curTotalHeapSize += curNativeHeapSize_;
    }

    if ( lastTotalHeapSize_ > curTotalHeapSize 
         || !lastTotalHeapSize_ )
    {
        lastTotalHeapSize_ = curTotalHeapSize;
        return;
    }

    if ( JS::IsIncrementalGCInProgress( cx ) 
         || ( curTotalHeapSize - lastTotalHeapSize_ > k_HeapGrowthRateTrigger ) )
    {
        if ( curTotalHeapSize <= k_MaxHeapSize / 2 )
        {
            if ( !JS::IsIncrementalGCInProgress( cx ) )
            {
                JS::PrepareZoneForGC( js::GetCompartmentZone( js::GetContextCompartment( cx ) ) );
                JS::StartIncrementalGC( cx, GC_NORMAL, JS::gcreason::RESERVED1, k_GcSliceTimeBudget );
            }
            else
            {
                JS::PrepareForIncrementalGC( cx );
                JS::IncrementalGCSlice( cx, JS::gcreason::RESERVED2, k_GcSliceTimeBudget );
            }
        }
        else
        {
            // A hack to make sure we never exceed the runtime size because we can't collect the memory
            // fast enough.
            if ( JS::IsIncrementalGCInProgress( cx ) )
            {
                JS::PrepareForIncrementalGC( cx );
                JS::FinishIncrementalGC( cx, JS::gcreason::RESERVED3 );
            }
            else
            {
                if ( curTotalHeapSize > k_MaxHeapSize * 0.75 )
                {
                    JS_SetGCParameter( cx, JSGC_MODE, JSGC_MODE_GLOBAL );
                    JS::PrepareForFullGC( cx );
                    JS::GCForReason( cx, GC_SHRINK, JS::gcreason::RESERVED4 );
                    JS_SetGCParameter( cx, JSGC_MODE, JSGC_MODE_INCREMENTAL );
                }
                else
                {
                    JS_GC( cx );
                }
            }
        }

        lastTotalHeapSize_ = curTotalHeapSize;
    }
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

}
