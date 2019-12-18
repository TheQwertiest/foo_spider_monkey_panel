#include <stdafx.h>

#include "js_engine.h"

#include <js_engine/js_container.h>
#include <js_engine/js_compartment_inner.h>
#include <js_engine/js_internal_global.h>
#include <js_objects/global_object.h>
#include <js_utils/js_error_helper.h>
#include <utils/error_popup.h>
#include <utils/scope_helpers.h>
#include <utils/string_helpers.h>
#include <utils/thread_helpers.h>

#include <adv_config.h>
#include <heartbeat_window.h>
#include <host_timer_dispatcher.h>
#include <js_panel_window.h>
#include <message_blocking_scope.h>
#include <user_message.h>

#include <js/Initialization.h>

using namespace smp;

namespace
{

constexpr uint32_t kHeartbeatRateMs = 73;
[[maybe_unused]] constexpr uint32_t kJobsMaxBudgetMs = 500;

} // namespace

namespace
{

void ReportException( const std::u8string& errorText )
{
    const std::u8string errorTextPadded = [&errorText]() {
        std::u8string text = "Critical JS engine error: " SMP_NAME_WITH_VERSION;
        if ( !errorText.empty() )
        {
            text += "\n";
            text += errorText;
        }

        return text;
    }();

    smp::utils::ReportErrorWithPopup( errorTextPadded );
}

} // namespace

namespace mozjs
{

JsEngine::JsEngine()
{
    JS_Init();
}

JsEngine::~JsEngine()
{ // Can't clean up here, since mozjs.dll might be already unloaded
    assert( !isInitialized_ );
}

JsEngine& JsEngine::GetInstance()
{
    static JsEngine je;
    return je;
}

void JsEngine::PrepareForExit()
{
    shouldShutdown_ = true;
}

bool JsEngine::RegisterContainer( JsContainer& jsContainer )
{
    if ( registeredContainers_.empty() && !Initialize() )
    {
        return false;
    }

    jsContainer.SetJsCtx( pJsCtx_ );

    assert( !registeredContainers_.count( &jsContainer ) );
    registeredContainers_.emplace( &jsContainer, jsContainer );

    jsMonitor_.AddContainer( jsContainer );

    return true;
}

void JsEngine::UnregisterContainer( JsContainer& jsContainer )
{
    if ( auto it = registeredContainers_.find( &jsContainer );
         it != registeredContainers_.end() )
    {
        jsMonitor_.RemoveContainer( jsContainer );

        it->second.get().Finalize();
        registeredContainers_.erase( it );
    }

    if ( registeredContainers_.empty() )
    {
        Finalize();
    }
}

void JsEngine::MaybeRunJobs()
{
    if ( !isInitialized_ )
    {
        return;
    }

    JSAutoRequest ar( pJsCtx_ );

    // TODO: add ability for user to abort script here

    if ( areJobsInProgress_ )
    { // might occur when called from nested loop
        /*
        // Use this only for script abort with error
        if ( timeGetTime() - jobsStartTime_ >= kJobsMaxBudgetMs )
        {
            js::StopDrainingJobQueue( pJsCtx_ );
        }
        */
        return;
    }

    jobsStartTime_ = timeGetTime();
    areJobsInProgress_ = true;

    {
        js::RunJobs( pJsCtx_ );

        for ( size_t i = 0; i < rejectedPromises_.length(); ++i )
        {
            const auto& rejectedPromise = rejectedPromises_[i];
            if ( !rejectedPromise )
            {
                continue;
            }

            JSAutoCompartment ac( pJsCtx_, rejectedPromise );
            mozjs::error::AutoJsReport are( pJsCtx_ );

            JS::RootedValue jsValue( pJsCtx_, JS::GetPromiseResult( rejectedPromise ) );
            if ( !jsValue.isNullOrUndefined() )
            {
                JS_SetPendingException( pJsCtx_, jsValue );
            }
            else
            { // Should not reach here, mostly paranoia check
                JS_ReportErrorUTF8( pJsCtx_, "Unhandled promise rejection" );
            }
        }
        rejectedPromises_.get().clear();
    }

    areJobsInProgress_ = false;
}

void JsEngine::OnJsActionStart( JsContainer& jsContainer )
{
    jsMonitor_.OnJsActionStart( jsContainer );
}

void JsEngine::OnJsActionEnd( JsContainer& jsContainer )
{
    jsMonitor_.OnJsActionEnd( jsContainer );
}

JsGc& JsEngine::GetGcEngine()
{
    return jsGc_;
}

const JsGc& JsEngine::GetGcEngine() const
{
    return jsGc_;
}

JsInternalGlobal& JsEngine::GetInternalGlobal()
{
    assert( internalGlobal_ );
    return *internalGlobal_;
}

void JsEngine::OnHeartbeat()
{
    if ( !isInitialized_ || isBeating_ || shouldStopHeartbeatThread_ )
    {
        return;
    }

    isBeating_ = true;

    {
        JSAutoRequest ar( pJsCtx_ );
        if ( !jsGc_.MaybeGc() )
        { // OOM
            ReportOomError();
        }
    }

    isBeating_ = false;
}

bool JsEngine::Initialize()
{
    if ( isInitialized_ )
    {
        return true;
    }

    utils::unique_ptr<JSContext> autoJsCtx( nullptr, []( auto pCtx ) {
        JS_DestroyContext( pCtx );
    } );

    try
    {
        autoJsCtx.reset( JS_NewContext( JsGc::GetMaxHeap() ) );
        SmpException::ExpectTrue( autoJsCtx.get(), "JS_NewContext failed" );

        JSContext* cx = autoJsCtx.get();

        if ( !JS_AddInterruptCallback( cx, InterruptHandler ) )
        {
            throw smp::JsException();
        }

        if ( !js::UseInternalJobQueues( cx ) )
        {
            throw smp::JsException();
        }

        JS::SetPromiseRejectionTrackerCallback( cx, RejectedPromiseHandler, this );

        // TODO: JS::SetWarningReporter( pJsCtx_ )

        if ( !JS::InitSelfHostedCode( cx ) )
        {
            throw smp::JsException();
        }

        jsGc_.Initialize( cx );

        rejectedPromises_.init( cx, JS::GCVector<JSObject*, 0, js::SystemAllocPolicy>( js::SystemAllocPolicy() ) );

        internalGlobal_ = JsInternalGlobal::Create( cx );
        assert( internalGlobal_ );

        StartHeartbeatThread();
        jsMonitor_.Start( cx );
    }
    catch ( const smp::JsException& )
    {
        assert( JS_IsExceptionPending( autoJsCtx.get() ) );
        ReportException( mozjs::error::JsErrorToText( autoJsCtx.get() ) );
        return false;
    }
    catch ( const SmpException& e )
    {
        ReportException( e.what() );
        return false;
    }

    pJsCtx_ = autoJsCtx.release();
    isInitialized_ = true;
    return true;
}

void JsEngine::Finalize()
{
    if ( pJsCtx_ )
    {
        jsMonitor_.Stop();
        // Stop the thread first, so that we don't get additional GC's during jsGc.Finalize
        StopHeartbeatThread();
        jsGc_.Finalize();

        internalGlobal_.reset();
        rejectedPromises_.reset();

        JS_DestroyContext( pJsCtx_ );
        pJsCtx_ = nullptr;
    }

    if ( shouldShutdown_ )
    {
        HostTimerDispatcher::Get().Finalize();
        JS_ShutDown();
    }

    isInitialized_ = false;
}

void JsEngine::StartHeartbeatThread()
{
    if ( !heartbeatWindow_ )
    {
        heartbeatWindow_ = smp::HeartbeatWindow::Create();
        assert( heartbeatWindow_ );
    }

    shouldStopHeartbeatThread_ = false;
    heartbeatThread_ = std::thread( [parent = this] {
        while ( !parent->shouldStopHeartbeatThread_ )
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds( kHeartbeatRateMs ) );

            PostMessage( parent->heartbeatWindow_->GetHwnd(), static_cast<UINT>( smp::MiscMessage::heartbeat ), 0, 0 );
        }
    } );
    smp::utils::SetThreadName( heartbeatThread_, "SMP Heartbeat" );
}

void JsEngine::StopHeartbeatThread()
{
    if ( heartbeatThread_.joinable() )
    {
        shouldStopHeartbeatThread_ = true;
        heartbeatThread_.join();
    }
}

bool JsEngine::InterruptHandler( JSContext* )
{
    return JsEngine::GetInstance().OnInterrupt();
}

bool JsEngine::OnInterrupt()
{
    return jsMonitor_.OnInterrupt();
}

void JsEngine::RejectedPromiseHandler( JSContext*, JS::HandleObject promise, JS::PromiseRejectionHandlingState state, void* data )
{
    JsEngine& self = *reinterpret_cast<JsEngine*>( data );

    if ( JS::PromiseRejectionHandlingState::Handled == state )
    {
        auto& uncaughtRejections = self.rejectedPromises_;
        for ( size_t i = 0; i < uncaughtRejections.length(); ++i )
        {
            if ( uncaughtRejections[i] == promise )
            {
                // To avoid large amounts of memmoves, we don't shrink the vector here.
                // Instead, we filter out nullptrs when iterating over the vector later.
                uncaughtRejections[i].set( nullptr );
                break;
            }
        }
    }
    else
    {
        self.rejectedPromises_.get().append( promise );
    }
}

void JsEngine::ReportOomError()
{
    for ( auto& [hWnd, jsContainer]: registeredContainers_ )
    {
        auto& jsContainerRef = jsContainer.get();
        if ( mozjs::JsContainer::JsStatus::Working != jsContainerRef.GetStatus() )
        {
            continue;
        }

        jsContainerRef.Fail( fmt::format( "Out of memory: {}/{} bytes", jsContainerRef.pNativeCompartment_->GetCurrentHeapBytes(), jsGc_.GetMaxHeap() ) );
    }
}

} // namespace mozjs
