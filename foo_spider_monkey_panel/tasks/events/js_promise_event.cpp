#include <stdafx.h>

#include "js_promise_event.h"

#include <js_backend/utils/js_error_helper.h>

SMP_MJS_SUPPRESS_WARNINGS_PUSH
#include <js/Promise.h>
SMP_MJS_SUPPRESS_WARNINGS_POP

namespace smp
{

JsPromiseEvent::JsPromiseEvent( JSContext* pJsCtx, mozjs::HeapDataHolder_Object heapHolder, std::function<JS::Value()> promiseResolver )
    : JsRunnableEvent( std::move( heapHolder ) )
    , pJsCtx_( pJsCtx )
    , promiseResolver_( promiseResolver )
{
}

JsPromiseEvent::JsPromiseEvent( JSContext* pJsCtx, mozjs::HeapDataHolder_Object heapHolder, std::exception_ptr pException )
    : JsRunnableEvent( std::move( heapHolder ) )
    , pJsCtx_( pJsCtx )
    , pException_( pException )
{
}

void JsPromiseEvent::RunJs()
{
    assert( JS::GetCurrentRealmOrNull( pJsCtx_ ) );
    JS::RootedObject jsPromise( pJsCtx_, GetJsTarget() );

    try
    {
        if ( pException_ )
        {
            std::rethrow_exception( pException_ );
        }

        JS::RootedValue jsPromiseResult( pJsCtx_, promiseResolver_() );
        (void)JS::ResolvePromise( pJsCtx_, jsPromise, jsPromiseResult );
    }
    catch ( ... )
    {
        mozjs::error::ExceptionToJsError( pJsCtx_ );

        JS::RootedValue jsError( pJsCtx_ );
        (void)JS_GetPendingException( pJsCtx_, &jsError );

        (void)JS::RejectPromise( pJsCtx_, jsPromise, jsError );
    }
}

} // namespace smp
