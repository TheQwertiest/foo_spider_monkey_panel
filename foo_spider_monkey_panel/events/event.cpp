#include <stdafx.h>

#include "event.h"

#include <panel/js_panel_window.h>

namespace smp
{

EventBase::EventBase( EventId id )
    : id_( id )
{
}

std::unique_ptr<EventBase> EventBase::Clone()
{
    return nullptr;
}

void EventBase::SetTarget( std::shared_ptr<PanelTarget> pTarget )
{
    assert( pTarget );
    pTarget_ = pTarget;
}

EventId EventBase::GetId() const
{
    return id_;
}

Event_Mouse* EventBase::AsMouseEvent()
{
    return nullptr;
}

PanelTarget::PanelTarget( panel::js_panel_window& panel )
    : pPanel_( &panel )
{
}

panel::js_panel_window* PanelTarget::GetPanel()
{
    assert( core_api::is_main_thread() );
    return pPanel_;
}

void PanelTarget::UnlinkPanel()
{
    pPanel_ = nullptr;
}

} // namespace smp
