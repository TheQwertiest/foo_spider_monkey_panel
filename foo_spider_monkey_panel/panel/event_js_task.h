#pragma once

#include <panel/event.h>
#include <panel/ievent_js_forwarder.h>

namespace mozjs
{

class JsAsyncTask;
class JsContainer;

} // namespace mozjs

namespace smp::panel
{

class js_panel_window;

class Event_JsTask : public Runnable
    , public IEvent_JsTask
{
public:
    Event_JsTask( EventId id, std::shared_ptr<mozjs::JsAsyncTask> pTask );

    void Run( js_panel_window& panelWindow ) override;

    std::optional<bool> JsExecute( mozjs::JsContainer& jsContainer ) override;

    Event_Mouse* AsMouseEvent() override;
    Event_Focus* AsFocusEvent() override;

private:
    const EventId id_;
    std::shared_ptr<mozjs::JsAsyncTask> pTask_;
};

} // namespace smp::panel
