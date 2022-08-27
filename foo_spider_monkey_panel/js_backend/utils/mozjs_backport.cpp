#include <stdafx.h>

#include "mozjs_backport.h"

SMP_MJS_SUPPRESS_WARNINGS_PUSH
#include <js/Promise.h>
SMP_MJS_SUPPRESS_WARNINGS_POP

namespace mozjs::backport
{

bool OnModuleEvaluationFailureSync( JSContext* cx,
                                    JS::HandleObject evaluationPromise )
{
    if ( evaluationPromise == nullptr )
    {
        return false;
    }

    // To allow module evaluation to happen synchronously throw the error
    // immediately. This assumes that any error will already have caused the
    // promise to be rejected, and doesn't support top-level await.
    JS::PromiseState state = JS::GetPromiseState( evaluationPromise );
    MOZ_DIAGNOSTIC_ASSERT( state == JS::PromiseState::Rejected || state == JS::PromiseState::Fulfilled );

    JS::SetSettledPromiseIsHandled( cx, evaluationPromise );
    if ( state == JS::PromiseState::Fulfilled )
    {
        return true;
    }

    JS::RootedValue error( cx, JS::GetPromiseResult( evaluationPromise ) );
    JS_SetPendingException( cx, error );
    return false;
}

} // namespace mozjs::backport
