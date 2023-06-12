#include <stdafx.h>

#include "event.h"

#include <panel/panel_accessor.h>
#include <panel/panel_window.h>

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

void EventBase::SetTarget( smp::not_null_shared<panel::PanelAccessor> pTarget )
{
    pTarget_ = pTarget;
}

EventId EventBase::GetId() const
{
    return id_;
}

const qwr::u8string& EventBase::GetType() const
{
    // TODO: map id to string
    static qwr::u8string tmp;
    return tmp;
}

Event_Mouse* EventBase::AsMouseEvent()
{
    return nullptr;
}

Event_Drag* EventBase::AsDragEvent()
{
    return nullptr;
}

} // namespace smp
