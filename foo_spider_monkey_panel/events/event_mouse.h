#pragma once

#include <events/event.h>
#include <events/event_js_executor.h>

namespace smp
{

class js_panel_window;

class Event_Mouse
    : public Event_JsExecutor
{
public:
    Event_Mouse( EventId id, int32_t x, int32_t y, uint32_t mask, uint32_t modifiers );

    [[nodiscard]] Event_Mouse* AsMouseEvent() override;

    std::optional<bool> JsExecute( mozjs::JsContainer& jsContainer ) override;

    [[nodiscard]] int32_t GetX() const;
    [[nodiscard]] int32_t GetY() const;
    [[nodiscard]] uint32_t GetMask() const;
    [[nodiscard]] uint32_t GetModifiers() const;
    [[nodiscard]] bool IsAltPressed() const;
    [[nodiscard]] bool IsCtrlPressed() const;
    [[nodiscard]] bool IsShiftPressed() const;
    [[nodiscard]] bool IsWinPressed() const;

private:
    const int32_t x_;
    const int32_t y_;
    const uint32_t mask_;
    const uint32_t modifiers_;
};

} // namespace smp
