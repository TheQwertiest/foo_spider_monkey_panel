#pragma once

#include <js_engine/js_container.h>
#include <panel/event.h>
#include <panel/ievent_js_forwarder.h>
#include <panel/js_panel_window.h>

namespace smp::panel
{

class Event_Mouse : public Runnable
    , public IEvent_JsTask
{
public:
    Event_Mouse( EventId id, int32_t x, int32_t y, uint32_t mask );

    void Run( js_panel_window& panelWindow ) override;

    std::optional<bool> JsExecute( mozjs::JsContainer& jsContainer ) override;

    Event_Mouse* AsMouseEvent() override;

    int32_t GetX() const;
    int32_t GetY() const;
    uint32_t GetMask() const;

private:
    const EventId id_;
    const int32_t x_;
    const int32_t y_;
    const uint32_t mask_;
};

} // namespace smp::panel
