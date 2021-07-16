#include <stdafx.h>

#include "event_js_task.h"

#include <js_engine/js_container.h>
#include <panel/js_panel_window.h>

namespace smp
{

Event_JsTask::Event_JsTask( EventId id, std::shared_ptr<mozjs::JsAsyncTask> pTask )
    : Event_JsExecutor( id )
    , pTask_( pTask )
{
    assert( pTask_ );
}

std::optional<bool> Event_JsTask::JsExecute( mozjs::JsContainer& jsContainer )
{
    jsContainer.InvokeJsAsyncTask( *pTask_ );
    return std::nullopt;
}

} // namespace smp
