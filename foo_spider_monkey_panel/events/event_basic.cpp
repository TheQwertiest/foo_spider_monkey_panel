#include <stdafx.h>

#include "event_basic.h"

#include <panel/js_panel_window.h>

namespace smp
{

Event_Basic::Event_Basic( EventId id )
    : EventBase( id )
{
}

void Event_Basic::Run()
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

    pPanel->ExecuteEvent_Basic( id_ );
}

} // namespace smp
