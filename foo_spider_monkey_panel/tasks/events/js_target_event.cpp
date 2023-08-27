#include <stdafx.h>

#include "js_target_event.h"

namespace smp
{

JsTargetEvent::JsTargetEvent( const qwr::u8string& type, JSContext* pJsCtx, JS::HandleObject jsTarget )
    : PanelEvent( EventId::kNew_JsTarget )
    , type_( type )
    , pHeapHelper_( std::make_shared<mozjs::HeapHelper>( pJsCtx ) )
    , jsTargetId_( pHeapHelper_->Store( jsTarget ) )
{
    assert( pJsCtx );
}

JsTargetEvent::JsTargetEvent( const qwr::u8string& type,
                              JSContext* pJsCtx,
                              const std::shared_ptr<mozjs::HeapHelper>& pHeapHelper,
                              uint32_t jsTargetId )
    : PanelEvent( EventId::kNew_JsTarget )
    , type_( type )
    , pHeapHelper_( pHeapHelper )
    , jsTargetId_( jsTargetId )
{
}

const qwr::u8string& JsTargetEvent::GetType() const
{
    return type_;
}

JSObject* JsTargetEvent::GetJsTarget()
{
    // this should be called only from valid js ctx
    assert( pHeapHelper_->IsJsAvailable() );
    return pHeapHelper_->GetObject( jsTargetId_ );
}

} // namespace smp
