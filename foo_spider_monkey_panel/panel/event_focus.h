#pragma once

#include <panel/event.h>
#include <panel/ievent_js_forwarder.h>

namespace mozjs
{

class JsContainer;

} // namespace mozjs

namespace smp::panel
{

class js_panel_window;

class Event_Focus : public Runnable
    , public IEvent_JsTask
{
public:
    Event_Focus( EventId id, bool isFocused );

    void Run( js_panel_window& panelWindow ) override;

    std::optional<bool> JsExecute( mozjs::JsContainer& jsContainer ) override;

    Event_Mouse* AsMouseEvent() override;
    Event_Focus* AsFocusEvent() override;

    bool IsFocused() const;

private:
    const EventId id_;
    const bool isFocused_;
};

} // namespace smp::panel
