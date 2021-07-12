#include <stdafx.h>

#include "event_js_task.h"

#include <js_engine/js_container.h>
#include <panel/js_panel_window.h>

namespace smp::panel
{

Event_JsTask::Event_JsTask( EventId id, std::shared_ptr<mozjs::JsAsyncTask> pTask )
    : id_( id )
    , pTask_( pTask )
{
    assert( pTask_ );
}

void Event_JsTask::Run( js_panel_window& panelWindow )
{
    assert( core_api::is_main_thread() );
    panelWindow.ExecuteJsTask( id_, *this );
}

void Event_JsTask::JsExecute( mozjs::JsContainer& jsContainer )
{
    jsContainer.InvokeJsAsyncTask( *pTask_ );
}

} // namespace smp::panel
