#include <stdafx.h>

#include "event.h"

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

void EventBase::SetTarget( std::shared_ptr<PanelTarget> pTarget )
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

PanelTarget::PanelTarget( panel::PanelWindow& panel )
    : pPanel_( &panel )
    , hWnd_( panel.GetHWND() )
{
}

HWND PanelTarget::GetHwnd()
{
    return hWnd_;
}

panel::PanelWindow* PanelTarget::GetPanel()
{
    assert( core_api::is_main_thread() );
    return pPanel_;
}

void PanelTarget::UnlinkPanel()
{
    pPanel_ = nullptr;
    hWnd_ = nullptr;
}

} // namespace smp
