#pragma once

#include <events/event.h>

namespace mozjs
{

class JsContainer;

} // namespace mozjs

namespace smp
{

class js_panel_window;

class Event_Basic : public Runnable
{
public:
    Event_Basic( EventId id );

    void Run( panel::js_panel_window& panelWindow ) override;

private:
    const EventId id_;
};

} // namespace smp
