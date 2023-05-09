#include <stdafx.h>

#include "panel_event.h"

#include <panel/panel_window.h>

namespace smp
{

PanelEvent::PanelEvent( EventId id )
    : EventBase( id )
{
}

std::unique_ptr<EventBase> PanelEvent::Clone()
{
    return std::make_unique<PanelEvent>( id_ );
}

void PanelEvent::Run()
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

    pPanel->ExecuteEvent( *this );
}

} // namespace smp
