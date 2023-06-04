#include <stdafx.h>

#include "js_runnable_event.h"

namespace smp
{

JsRunnableEvent::JsRunnableEvent( JSContext* pJsCtx, JS::HandleObject jsTarget )
    : PanelEvent( EventId::kNew_JsRunnable )
    , pHeapHelper_( std::make_shared<mozjs::HeapHelper>( pJsCtx ) )
    , jsTargetId_( pHeapHelper_->Store( jsTarget ) )
{
    assert( pJsCtx );
}

JsRunnableEvent::JsRunnableEvent( JSContext* pJsCtx,
                                  const std::shared_ptr<mozjs::HeapHelper>& pHeapHelper,
                                  uint32_t jsTargetId )
    : PanelEvent( EventId::kNew_JsRunnable )
    , pHeapHelper_( pHeapHelper )
    , jsTargetId_( jsTargetId )
{
}

JSObject* JsRunnableEvent::GetJsTarget()
{
    // this should be called only from valid js ctx
    assert( pHeapHelper_->IsJsAvailable() );
    return pHeapHelper_->GetObject( jsTargetId_ );
}

} // namespace smp
