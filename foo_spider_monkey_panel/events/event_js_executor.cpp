#include <stdafx.h>

#include "event_js_executor.h"

#include <panel/js_panel_window.h>

namespace smp
{

Event_JsExecutor::Event_JsExecutor( EventId id )
    : EventBase( id )
{
}

void Event_JsExecutor::Run()
{
    if ( !pTarget_ )
    {
        return;
    }

    auto pPanel = pTarget_->GetPanel();
    if ( !pPanel )
    {
        return;
    }

    pPanel->ExecuteEvent_JsTask( id_, *this );
}

} // namespace smp
