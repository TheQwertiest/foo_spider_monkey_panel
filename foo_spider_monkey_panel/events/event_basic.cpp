#include <stdafx.h>

#include "event_basic.h"

#include <panel/js_panel_window.h>

namespace smp
{

Event_Basic::Event_Basic( EventId id )
    : id_( id )
{
}

void Event_Basic::Run( panel::js_panel_window& panelWindow )
{
    panelWindow.ExecuteTask( id_ );
}

} // namespace smp
