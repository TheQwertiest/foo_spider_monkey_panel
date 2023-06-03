#include <stdafx.h>

#include "micro_task.h"

#include <js_backend/utils/js_error_helper.h>

namespace smp
{

MicroTask::MicroTask( JSContext* pJsCtx, JS::HandleObject jsTarget, RunnerFn fn )
    : pJsCtx_( pJsCtx )
    , heapHelper_( pJsCtx )
    , jsTargetId_( heapHelper_.Store( jsTarget ) )
    , fn_( fn )
{
    assert( core_api::is_main_thread() );
    assert( pJsCtx );
}

void MicroTask::Run()
{
    assert( core_api::is_main_thread() );
    if ( !IsCallable() )
    {
        return;
    }

    // assumes that realm is set up by the caller
    assert( JS::GetCurrentRealmOrNull( pJsCtx_ ) );
    mozjs::error::AutoJsReport are( pJsCtx_ );

    JS::RootedObject jsTarget( pJsCtx_, GetJsTarget() );
    try
    {
        fn_( pJsCtx_, jsTarget );
    }
    catch ( ... )
    {
        mozjs::error::ExceptionToJsError( pJsCtx_ );
    }
}

JSObject* MicroTask::GetJsTarget()
{
    assert( core_api::is_main_thread() );
    assert( IsCallable() );
    return heapHelper_.GetObject( jsTargetId_ );
}

bool MicroTask::IsCallable() const
{
    assert( core_api::is_main_thread() );
    return ( !IsCancelled() && heapHelper_.IsJsAvailable() );
}

} // namespace smp
