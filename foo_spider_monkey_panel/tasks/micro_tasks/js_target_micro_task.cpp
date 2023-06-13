#include <stdafx.h>

#include "js_target_micro_task.h"

#include <js_backend/utils/js_error_helper.h>
#include <js_backend/utils/panel_from_global.h>

namespace smp
{

JsTargetMicroTask::JsTargetMicroTask( JSContext* pJsCtx, JS::HandleObject jsTarget, RunnerFn fn )
    : pJsCtx_( pJsCtx )
    , heapHelper_( pJsCtx )
    , jsTargetId_( heapHelper_.Store( jsTarget ) )
    , fn_( fn )

{
    assert( core_api::is_main_thread() );
}

void JsTargetMicroTask::Run()
{
    assert( core_api::is_main_thread() );
    if ( IsCancelled() || !heapHelper_.IsJsAvailable() )
    {
        return;
    }

    auto pTargetHeapObject = heapHelper_.GetObject( jsTargetId_ );

    JSAutoRealm ac( pJsCtx_, pTargetHeapObject );
    if ( !mozjs::HasHostPanelForCurrentGlobal( pJsCtx_ ) )
    {
        return;
    }

    mozjs::error::AutoReportError are( pJsCtx_ );
    try
    {
        JS::RootedObject jsTarget( pJsCtx_, pTargetHeapObject );
        fn_( pJsCtx_, jsTarget );
    }
    catch ( ... )
    {
        mozjs::error::ExceptionToJsError( pJsCtx_ );
    }
}

} // namespace smp
