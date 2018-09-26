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

    JSContext* pJsCtx = JS_NewContext( jsGc_.GetMaxHeap() );
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

    if ( !jsGc_.Initialize( pJsCtx ) )
    {// TODO: report
        return false;
    }

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
    jsGc_.MaybeGc();
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
