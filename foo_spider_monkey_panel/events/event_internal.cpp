#include <stdafx.h>

#include "event_internal.h"

#include <panel/js_panel_window.h>

namespace smp
{

Event_Internal::Event_Internal( EventId id )
    : id_( id )
{
}

void Event_Internal::Run( panel::js_panel_window& panelWindow )
{
    panelWindow.ExecuteInternalTask( id_ );
}

} // namespace smp
