#pragma once

#include <events/event.h>
#include <events/event_js_executor.h>

namespace mozjs
{

class JsContainer;

} // namespace mozjs

namespace smp
{

class js_panel_window;

class Event_Mouse
    : public Event_JsExecutor
{
public:
    Event_Mouse( EventId id, int32_t x, int32_t y, uint32_t mask );

    Event_Mouse* AsMouseEvent() override;

    std::optional<bool> JsExecute( mozjs::JsContainer& jsContainer ) override;

    int32_t GetX() const;
    int32_t GetY() const;
    uint32_t GetMask() const;

private:
    const int32_t x_;
    const int32_t y_;
    const uint32_t mask_;
};

} // namespace smp
