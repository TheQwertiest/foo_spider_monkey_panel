#pragma once

#include <events/panel_event.h>

#include <qwr/enum_bitmask_ops.hpp>

namespace smp
{

class MouseEventNew
    : public PanelEvent
{
public:
    enum class ModifierKeyFlag
    {
        kNoModifiers = 0,
        kAlt = 1,
        kCtrl = 1 << 1,
        kShift = 1 << 2,
        kWin = 1 << 3
    };
    friend QWR_ENABLE_BIT_OPS( ModifierKeyFlag );

    enum class MouseKeyFlag
    {
        kNoButtons = 0,
        kPrimary = 1 << 1,
        kSecondary = 1 << 2,
        kAuxiliary = 1 << 3,
        k4 = 1 << 4,
        k5 = 1 << 5
    };
    friend QWR_ENABLE_BIT_OPS( MouseKeyFlag );

public:
    MouseEventNew( EventId id,
                   MouseKeyFlag button,
                   int32_t x,
                   int32_t y,
                   int32_t prevX,
                   int32_t prevY );

    [[nodiscard]] int32_t GetX() const;
    [[nodiscard]] int32_t GetY() const;
    [[nodiscard]] int32_t GetPrevX() const;
    [[nodiscard]] int32_t GetPrevY() const;

    [[nodiscard]] MouseKeyFlag GetButton() const;
    [[nodiscard]] MouseKeyFlag GetButtons() const;
    [[nodiscard]] ModifierKeyFlag GetModifiers() const;

private:
    static MouseKeyFlag CalculateButtons();
    static ModifierKeyFlag CalculateModifiers();

private:
    const int32_t x_;
    const int32_t y_;
    const int32_t prevX_;
    const int32_t prevY_;
    MouseKeyFlag button_;
    MouseKeyFlag buttons_;
    ModifierKeyFlag modifiers_;
};

} // namespace smp
