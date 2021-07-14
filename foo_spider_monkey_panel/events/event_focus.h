#pragma once

#include <events/event.h>
#include <events/ievent_js_forwarder.h>

namespace mozjs
{

class JsContainer;

} // namespace mozjs

namespace smp
{

class js_panel_window;

class Event_Focus : public Runnable
    , public IEvent_JsTask
{
public:
    Event_Focus( EventId id, bool isFocused );

    void Run( panel::js_panel_window& panelWindow ) override;

    std::optional<bool> JsExecute( mozjs::JsContainer& jsContainer ) override;

    Event_Focus* AsFocusEvent() override;

    bool IsFocused() const;

private:
    const EventId id_;
    const bool isFocused_;
};

} // namespace smp
