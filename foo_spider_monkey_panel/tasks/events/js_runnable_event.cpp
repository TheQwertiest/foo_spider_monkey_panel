#include <stdafx.h>

#include "js_runnable_event.h"

namespace smp
{

JsRunnableEvent::JsRunnableEvent( mozjs::HeapDataHolder_Object heapHolder )
    : PanelEvent( EventId::kNew_JsRunnable )
    , heapHolder_( std::move( heapHolder ) )
{
}

JSObject* JsRunnableEvent::GetJsTarget()
{
    return heapHolder_.Get();
}

} // namespace smp
