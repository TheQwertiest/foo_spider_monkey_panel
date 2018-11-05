#include <stdafx.h>
#include "js_engine.h"

#include <js_engine/js_container.h>
#include <js_engine/js_compartment_inner.h>
#include <js_objects/global_object.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/scope_helper.h>
#include <utils/string_helpers.h>

#include <js_panel_window.h>
#include <adv_config.h>
#include <user_message.h>
#include <heartbeat_window.h>
#include <popup_msg.h>

#include <js/Initialization.h>

namespace
{

constexpr uint32_t kHeartbeatRateMs = 73;
constexpr uint32_t kJobsMaxBudgetMs = 500;

} // namespace

namespace
{
void ReportException( const pfc::string8_fast& errorText )
{
    const pfc::string8_fast errorTextPadded = [&errorText]() {
        pfc::string8_fast text = "Critical JS engine error: " SMP_NAME_WITH_VERSION;
        if ( !errorText.is_empty() )
        {
            text += "\n";
            text += errorText;
        }

        return text;
    }();

    FB2K_console_formatter() << errorTextPadded.c_str();
    popup_msg::g_show( errorTextPadded, SMP_NAME );
    MessageBeep( MB_ICONASTERISK );
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
    static JsEngine jsEnv;
    return jsEnv;
}

void JsEngine::PrepareForExit()
{
    shouldShutdown_ = true;
}

bool JsEngine::RegisterContainer( JsContainer& jsContainer )
{
    if ( !registeredContainers_.size() && !Initialize() )
    {
        return false;
    }

    jsContainer.SetJsCtx( pJsCtx_ );
    registeredContainers_.insert_or_assign( &jsContainer, jsContainer );

    return true;
}

void JsEngine::UnregisterContainer( JsContainer& jsContainer )
{
    auto elem = registeredContainers_.find( &jsContainer );
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

bool JsEngine::Initialize()
{
    if ( isInitialized_ )
    {
        return true;
    }

    scope::unique_ptr<JSContext> autoJsCtx( nullptr, []( auto pCtx ) {
        JS_DestroyContext( pCtx );
    } );

    try
    {
        autoJsCtx.reset( JS_NewContext( jsGc_.GetMaxHeap() ) );
        if ( !autoJsCtx )
        {
            throw smp::SmpException( "JS_NewContext failed" );
        }

        JSContext* cx = autoJsCtx.get();

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

        StartHeartbeatThread();
    }
    catch ( const smp::JsException& )
    {
        assert( JS_IsExceptionPending( autoJsCtx.get() ) );
        ReportException( error::JsErrorToText( autoJsCtx.get() ) );
        return false;
    }
    catch ( const smp::SmpException& e )
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
        StopHeartbeatThread();

        rejectedPromises_.reset();

        for ( auto& [hWnd, jsContainer] : registeredContainers_ )
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

const JsGc& JsEngine::GetGcEngine() const
{
    return jsGc_;
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
            auto& rejectedPromise = rejectedPromises_[i];
            if ( !rejectedPromise )
            {
                continue;
            }

            JSAutoCompartment ac( pJsCtx_, rejectedPromise );
            error::AutoJsReport are( pJsCtx_ );

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
}

void JsEngine::StopHeartbeatThread()
{
    if ( heartbeatThread_.joinable() )
    {
        shouldStopHeartbeatThread_ = true;
        heartbeatThread_.join();
    }
}

void JsEngine::RejectedPromiseHandler( JSContext* cx, JS::HandleObject promise, JS::PromiseRejectionHandlingState state, void* data )
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
    // A dirty way to link compartment with the corresponding container:
    // we can't GC in JS_IterateCompartments, so we are saving pointers to
    // native globals instead and comparing it against the one from container.

    struct OomData
    {
        OomData( void* pGlobal, uint32_t memory )
            : pGlobal( pGlobal )
            , memory( memory )
        {
        }
        void* pGlobal;
        uint32_t memory;
    };

    std::vector<OomData> oomData;
    oomData.reserve( registeredContainers_.size() );

    JS_IterateCompartments( pJsCtx_, &oomData, []( JSContext* cx, void* data, JSCompartment* pJsCompartment ) {
        std::vector<OomData>& oomData = *reinterpret_cast<std::vector<OomData>*>( data );

        auto pNativeCompartment = static_cast<JsCompartmentInner*>( JS_GetCompartmentPrivate( pJsCompartment ) );
        if ( !pNativeCompartment )
        {
            return;
        }

        JS::RootedObject jsGlobal( cx, JS_GetGlobalForCompartmentOrNull( cx, pJsCompartment ) );
        if ( !jsGlobal )
        {
            return;
        }

        auto pNativeGlobal = static_cast<JsGlobalObject*>( JS_GetPrivate( jsGlobal ) );
        if ( !pNativeGlobal )
        {
            return;
        }

        oomData.emplace_back( pNativeGlobal, pNativeCompartment->GetCurrentHeapBytes() );
    } );

    for ( auto& [hWnd, jsContainer] : registeredContainers_ )
    {
        if ( mozjs::JsContainer::JsStatus::Working != jsContainer.get().GetStatus() )
        {
            continue;
        }

        auto it = std::find_if(
            oomData.cbegin(),
            oomData.cend(),
            [pNativeGlobal = jsContainer.get().pNativeGlobal_]( const auto& oomDataElem ) {
                return oomDataElem.pGlobal == pNativeGlobal;
            } );

        std::string errorMsg = "Out of memory";
        if ( oomData.cend() != it )
        {
            errorMsg += smp::string::Formatter() << ": " << it->memory << "/" << jsGc_.GetMaxHeap() << " bytes";
        }

        jsContainer.get().Fail( errorMsg.c_str() );
    }
}

} // namespace mozjs
