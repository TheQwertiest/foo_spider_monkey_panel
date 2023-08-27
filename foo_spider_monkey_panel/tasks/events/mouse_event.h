#pragma once

#include <tasks/events/panel_event.h>

#include <qwr/enum_bitmask_ops.hpp>

namespace smp
{

class MouseEvent
    : public PanelEvent
{
public:
    enum class ModifierFlag
    {
        kNoModifiers = 0,
        kAlt = 1,
        kCtrl = 1 << 1,
        kShift = 1 << 2,
        kWin = 1 << 3
    };
    friend QWR_ENABLE_BIT_OPS( ModifierFlag );

    enum class KeyFlag
    {
        kNoButtons = 0,
        kPrimary = 1 << 1,
        kSecondary = 1 << 2,
        kAuxiliary = 1 << 3,
        k4 = 1 << 4,
        k5 = 1 << 5
    };
    friend QWR_ENABLE_BIT_OPS( KeyFlag );

public:
    [[nodiscard]] MouseEvent( EventId id,
                              KeyFlag button,
                              int32_t x,
                              int32_t y );

    [[nodiscard]] int32_t GetX() const;
    [[nodiscard]] int32_t GetY() const;

    [[nodiscard]] KeyFlag GetButton() const;
    [[nodiscard]] KeyFlag GetButtons() const;
    [[nodiscard]] ModifierFlag GetModifiers() const;

private:
    static KeyFlag CalculateButtons();
    static ModifierFlag CalculateModifiers();

private:
    const int32_t x_;
    const int32_t y_;
    KeyFlag button_;
    KeyFlag buttons_;
    ModifierFlag modifiers_;
};

} // namespace smp
