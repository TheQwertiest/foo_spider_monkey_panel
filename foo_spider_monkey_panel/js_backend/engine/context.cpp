#include <stdafx.h>

#include "context.h"

#include <js_backend/engine/engine.h>
#include <js_backend/engine/global_heap_manager.h>
#include <js_backend/engine/heartbeat_window.h>
#include <js_backend/engine/js_container.h>
#include <js_backend/engine/js_realm_inner.h>
#include <js_backend/engine/js_script_cache.h>
#include <js_backend/utils/js_heap_helper.h>
#include <js_backend/utils/panel_from_global.h>
#include <panel/user_message.h>
#include <tasks/micro_tasks/micro_task_runnable.h>
#include <utils/make_unique_ptr.h>

#include <js/Initialization.h>
#include <js/Modules.h>
#include <qwr/error_popup.h>
#include <qwr/final_action.h>
#include <qwr/thread_name_setter.h>

using namespace smp;

namespace
{

constexpr uint32_t kHeartbeatRateMs = 73;
[[maybe_unused]] constexpr uint32_t kJobsMaxBudgetMs = 500;

// Half the size of the actual C stack, to be safe (default stack size in VS is 1MB).
constexpr size_t kMaxStackLimit = 1024LL * 1024 / 2;

thread_local mozjs::ContextInner* pThreadLocalContext = nullptr;

} // namespace

namespace
{

class ReportExceptionClosure final
    : public js::ScriptEnvironmentPreparer::Closure
{

public:
    explicit ReportExceptionClosure( JS::HandleValue exn )
        : exn_( exn )
    {
    }

    bool operator()( JSContext* cx ) final
    {
        JS_SetPendingException( cx, exn_ );
        return true;
    }

private:
    JS::HandleValue exn_;
};

class PromiseJobRunnable final : public smp::MicroTaskRunnable
{
public:
    PromiseJobRunnable( JSContext* cx, JS::HandleObject job )
        : pJsCtx_( cx )
        , heapHelper_( cx )
        , jsTargetId_( heapHelper_.Store( job ) )
    {
    }

    void Run() final
    { // based on https://searchfox.org/mozilla-central/source/js/src/vm/JSContext.h :: InternalJobQueue
        assert( core_api::is_main_thread() );

        if ( !heapHelper_.IsJsAvailable() )
        {
            return;
        }

        auto pJobHeapObject = heapHelper_.GetObject( jsTargetId_ );

        JSAutoRealm ac( pJsCtx_, pJobHeapObject );
        if ( !mozjs::HasHostPanelForCurrentGlobal( pJsCtx_ ) )
        {
            return;
        }

        JS::RootedObject jsGlobal( pJsCtx_, JS::CurrentGlobalOrNull( pJsCtx_ ) );
        assert( jsGlobal );

        JS::RootedObject jsJob( pJsCtx_, pJobHeapObject );
        JS::HandleValueArray args( JS::HandleValueArray::empty() );
        JS::RootedValue rval( pJsCtx_ );
        if ( JS::Call( pJsCtx_, JS::UndefinedHandleValue, jsJob, args, &rval ) )
        {
            return;
        }

        if ( !JS_IsExceptionPending( pJsCtx_ ) )
        {
            return;
        }

        JS::RootedValue exn( pJsCtx_ );
        if ( !JS_GetPendingException( pJsCtx_, &exn ) )
        {
            return;
        }

        JS_ClearPendingException( pJsCtx_ );
        ::ReportExceptionClosure reportExn( exn );
        js::PrepareScriptEnvironmentAndInvoke( pJsCtx_, jsGlobal, reportExn );
    }

private:
    JSContext* pJsCtx_ = nullptr;
    mozjs::HeapHelper heapHelper_;
    const uint32_t jsTargetId_;
};

class SavedQueue : public JS::JobQueue::SavedJobQueue
{
public:
    SavedQueue( std::queue<smp::not_null_shared<smp::MicroTaskRunnable>>&& microTaskQueue,
                bool isDraining )
        : microTaskQueue_( std::move( microTaskQueue ) )
        , isDraining_( isDraining )
    {
    }

    ~SavedQueue()
    {
        auto& ctxInner = mozjs::ContextInner::Get();
        ctxInner.microTaskQueue_ = std::move( microTaskQueue_ );
        ctxInner.isDrainingMicroTasks_ = isDraining_;
    }

private:
    std::queue<smp::not_null_shared<smp::MicroTaskRunnable>> microTaskQueue_;
    bool isDraining_ = false;
};

} // namespace

namespace
{

void ReportException( const qwr::u8string& errorText )
{
    const auto errorTextPadded = [&errorText] {
        qwr::u8string text = "Critical JS engine error: " SMP_NAME_WITH_VERSION;
        if ( !errorText.empty() )
        {
            text += "\n";
            text += errorText;
        }

        return text;
    }();

    qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, errorTextPadded );
}

} // namespace

namespace mozjs
{

ContextInner::ContextInner()
{
}

ContextInner::~ContextInner()
{
}

ContextInner& ContextInner::Get()
{
    assert( pThreadLocalContext );
    return *pThreadLocalContext;
}

void ContextInner::PrepareForExit()
{
    if ( registeredContainers_.empty() )
    { // finalize immediately, since we don't have containers to care about
        Finalize();
    }
}

bool ContextInner::RegisterContainer( JsContainer& jsContainer )
{
    if ( registeredContainers_.empty() && !Initialize() )
    {
        return false;
    }

    jsContainer.SetJsCtx( pJsCtx_ );

    assert( !registeredContainers_.contains( &jsContainer ) );
    registeredContainers_.emplace( &jsContainer, jsContainer );

    jsMonitor_.AddContainer( jsContainer );

    return true;
}

void ContextInner::UnregisterContainer( JsContainer& jsContainer )
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

void ContextInner::OnJsActionStart( JsContainer& jsContainer )
{
    jsMonitor_.OnJsActionStart( jsContainer );
}

void ContextInner::OnJsActionEnd( JsContainer& jsContainer )
{
    jsMonitor_.OnJsActionEnd( jsContainer );
}

void ContextInner::EnqueueMicroTask( smp::not_null_shared<smp::MicroTaskRunnable> pMicroTask )
{
    if ( !isInitialized_ )
    {
        return;
    }

    JS::JobQueueMayNotBeEmpty( pJsCtx_ );
    microTaskQueue_.emplace( pMicroTask );
}

void ContextInner::PerformMicroTaskCheckPoint()
{
    if ( !isInitialized_ )
    {
        return;
    }

    // TODO: add ability for user to abort script here

    if ( inMicroTaskCheckpoint_ )
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
    inMicroTaskCheckpoint_ = true;
    const auto autoJobs = qwr::final_action( [&] { inMicroTaskCheckpoint_ = false; } );

    {
        js::RunJobs( pJsCtx_ );

        for ( size_t i = 0; i < rejectedPromises_.length(); ++i )
        {
            const auto& rejectedPromise = rejectedPromises_[i];
            if ( !rejectedPromise )
            {
                continue;
            }

            JSAutoRealm ac( pJsCtx_, rejectedPromise );
            mozjs::error::AutoReportError are( pJsCtx_ );

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
}

JsGc& ContextInner::GetGcEngine()
{
    return jsGc_;
}

const JsGc& ContextInner::GetGcEngine() const
{
    return jsGc_;
}

JsScriptCache& ContextInner::GetScriptCache()
{
    assert( pScriptCache_ );
    return *pScriptCache_;
}

void ContextInner::OnHeartbeat()
{
    if ( !isInitialized_ || isBeating_
         || !pHeartbeatThread_ || pHeartbeatThread_->get_stop_token().stop_requested() )
    {
        return;
    }

    isBeating_ = true;

    {
        if ( !jsGc_.MaybeGc() )
        { // OOM
            ReportOomError();
        }
    }

    isBeating_ = false;
}

JSObject* ContextInner::getIncumbentGlobal( JSContext* cx )
{
    return JS::CurrentGlobalOrNull( cx );
}

bool ContextInner::enqueuePromiseJob( JSContext* cx, JS::HandleObject /*promise*/, JS::HandleObject job, JS::HandleObject /*allocationSite*/, JS::HandleObject /*incumbentGlobal*/ )
{
    EnqueueMicroTask( smp::make_not_null( std::make_shared<PromiseJobRunnable>( cx, job ) ) );
    return true;
}

void ContextInner::runJobs( JSContext* cx )
{
    if ( isDrainingMicroTasks_ || isRunMicroTasksInterrupted_ )
    {
        return;
    }

    isDrainingMicroTasks_ = true;
    qwr::final_action autoDrain( [&] { isDrainingMicroTasks_ = false; } );

    while ( !microTaskQueue_.empty() )
    {
        if ( isRunMicroTasksInterrupted_ )
        {
            isRunMicroTasksInterrupted_ = false;
            isDrainingMicroTasks_ = false;
            break;
        }

        auto pMicroTask = microTaskQueue_.front();
        microTaskQueue_.pop();

        if ( microTaskQueue_.empty() )
        {
            JS::JobQueueIsEmpty( cx );
        }

        pMicroTask->Run();
    }
}

bool ContextInner::empty() const
{
    return microTaskQueue_.empty();
}

js::UniquePtr<JS::JobQueue::SavedJobQueue> ContextInner::saveJobQueue( JSContext* cx )
{
    auto pSavedQueue = js::MakeUnique<SavedQueue>( std::move( microTaskQueue_ ), isDrainingMicroTasks_ );
    if ( !pSavedQueue )
    {
        js::ReportOutOfMemory( cx );
        return nullptr;
    }

    ClearJobQueue();

    isDrainingMicroTasks_ = false;
    return pSavedQueue;
}

void ContextInner::ClearJobQueue()
{
    decltype( microTaskQueue_ ) tmp;
    std::swap( microTaskQueue_, tmp );
}

bool ContextInner::Initialize()
{
    if ( isInitialized_ )
    {
        return true;
    }

    auto autoJsCtx = smp::utils::make_unique_with_dtor<JSContext>( nullptr, []( auto pCtx ) { JS_DestroyContext( pCtx ); } );

    try
    {
        autoJsCtx.reset( JS_NewContext( JsGc::GetMaxHeap() ) );
        qwr::QwrException::ExpectTrue( autoJsCtx.get(), "JS_NewContext failed" );

        JSContext* cx = autoJsCtx.get();

        JS_SetContextPrivate( cx, this );

        JS_SetNativeStackQuota( cx, kMaxStackLimit );
        JS_SetGlobalJitCompilerOption( cx, JSJITCOMPILER_SPECTRE_INDEX_MASKING, 0 );
        JS_SetGlobalJitCompilerOption( cx, JSJITCOMPILER_SPECTRE_JIT_TO_CXX_CALLS, 0 );
        JS_SetGlobalJitCompilerOption( cx, JSJITCOMPILER_SPECTRE_STRING_MITIGATIONS, 0 );
        JS_SetGlobalJitCompilerOption( cx, JSJITCOMPILER_SPECTRE_VALUE_MASKING, 0 );
        JS_SetGlobalJitCompilerOption( cx, JSJITCOMPILER_SPECTRE_JIT_TO_CXX_CALLS, 0 );

        if ( !JS_AddInterruptCallback( cx, InterruptHandler ) )
        {
            throw JsException();
        }

        JS::SetJobQueue( cx, this );
        JS::SetPromiseRejectionTrackerCallback( cx, RejectedPromiseHandler, this );

        JS::SetModuleResolveHook( JS_GetRuntime( cx ), ModuleResolver );
        JS::SetModuleMetadataHook( JS_GetRuntime( cx ), ModuleMetaGenerator );

        // TODO: JS::SetWarningReporter( pJsCtx_ )

        if ( !JS::InitSelfHostedCode( cx ) )
        {
            throw JsException();
        }

        jsGc_.Initialize( cx );

        rejectedPromises_.init( cx, JS::GCVector<JSObject*, 0, js::SystemAllocPolicy>( js::SystemAllocPolicy() ) );

        pScriptCache_ = std::make_unique<JsScriptCache>();

        StartHeartbeatThread();
        jsMonitor_.Start( cx );
    }
    catch ( const JsException& /*e*/ )
    {
        assert( JS_IsExceptionPending( autoJsCtx.get() ) );
        ReportException( mozjs::error::JsErrorToText( autoJsCtx.get() ) );
        return false;
    }
    catch ( const qwr::QwrException& e )
    {
        ReportException( e.what() );
        return false;
    }

    pJsCtx_ = autoJsCtx.release();
    pThreadLocalContext = this;
    isInitialized_ = true;

    return true;
}

void ContextInner::Finalize()
{
    if ( pJsCtx_ )
    {
        jsMonitor_.Stop();
        // Stop the thread first, so that we don't get additional GC's during jsGc.Finalize
        StopHeartbeatThread();
        jsGc_.Finalize();

        pScriptCache_.reset();
        rejectedPromises_.reset();
        ClearJobQueue();

        JS_DestroyContext( pJsCtx_ );
        pJsCtx_ = nullptr;
        pThreadLocalContext = nullptr;
    }

    JsEngine::Get().Finalize();
    isInitialized_ = false;
}

void ContextInner::StartHeartbeatThread()
{
    if ( !heartbeatWindow_ )
    {
        heartbeatWindow_ = smp::HeartbeatWindow::Create();
        assert( heartbeatWindow_ );
    }

    assert( !pHeartbeatThread_ );
    pHeartbeatThread_ = std::make_unique<std::jthread>( [&]( std::stop_token token ) {
        while ( !token.stop_requested() )
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds( kHeartbeatRateMs ) );

            PostMessage( heartbeatWindow_->GetHwnd(), static_cast<UINT>( smp::MiscMessage::heartbeat ), 0, 0 );
        }
    } );
    qwr::SetThreadName( *pHeartbeatThread_, "SMP Heartbeat" );
}

void ContextInner::StopHeartbeatThread()
{
    if ( pHeartbeatThread_ )
    {
        pHeartbeatThread_->request_stop();
        pHeartbeatThread_.reset();
    }
}

bool ContextInner::InterruptHandler( JSContext* /*cx*/ )
{
    return ContextInner::Get().jsMonitor_.OnInterrupt();
}

void ContextInner::RejectedPromiseHandler( JSContext* /*cx*/, bool mutedErrors, JS::HandleObject promise, JS::PromiseRejectionHandlingState state, void* data )
{
    auto& self = *reinterpret_cast<ContextInner*>( data );

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

JSObject* ContextInner::ModuleResolver( JSContext* cx, JS::HandleValue modulePrivate, JS::HandleObject moduleRequest )
{
    try
    {
        JS::RootedString specifierString( cx, JS::GetModuleRequestSpecifier( cx, moduleRequest ) );
        assert( specifierString );
        const auto moduleName = convert::to_native::ToValue<qwr::u8string>( cx, specifierString );

        JS::RootedObject jsGlobal( cx, JS::CurrentGlobalOrNull( cx ) );
        assert( jsGlobal );

        const auto pNativeGlobal = JsGlobalObject::ExtractNative( cx, jsGlobal );
        assert( pNativeGlobal );

        return pNativeGlobal->GetScriptLoader().GetResolvedModule( moduleName, modulePrivate );
    }
    catch ( ... )
    {
        mozjs::error::ExceptionToJsError( cx );
        return nullptr;
    }
}

bool ContextInner::ModuleMetaGenerator( JSContext* cx, JS::HandleValue modulePrivate, JS::HandleObject metaObject )
{
    try
    {
        ScriptLoader::PopulateImportMeta( cx, modulePrivate, metaObject );
        return true;
    }
    catch ( ... )
    {
        mozjs::error::ExceptionToJsError( cx );
        return false;
    }
}

void ContextInner::ReportOomError()
{
    for ( auto& [hWnd, jsContainer]: registeredContainers_ )
    {
        auto& jsContainerRef = jsContainer.get();
        if ( mozjs::JsContainer::JsStatus::Working != jsContainerRef.GetStatus() )
        {
            continue;
        }

        jsContainerRef.Fail( fmt::format( "Out of memory: {}/{} bytes", jsContainerRef.pNativeRealm_->GetCurrentHeapBytes(), JsGc::GetMaxHeap() ) );
    }
}

} // namespace mozjs
