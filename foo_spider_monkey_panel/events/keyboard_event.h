#pragma once

#include <events/panel_event.h>

#include <qwr/enum_bitmask_ops.hpp>

namespace smp
{

class KeyboardEvent
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

    enum class KeyLocation
    {
        kStandard,
        kLeft,
        kRight,
        kNumpad
    };

public:
    [[nodiscard]] KeyboardEvent( EventId id,
                                 const std::wstring& chars,
                                 uint32_t virtualCode,
                                 uint32_t scanCode,
                                 bool isExtendedScanCode );

    // TODO: save all other data
    [[nodiscard]] const std::wstring& GetChars() const;
    [[nodiscard]] uint32_t GetVirtualCode() const;
    [[nodiscard]] uint32_t GetScanCode() const;
    [[nodiscard]] bool IsExtendedScanCode() const;
    [[nodiscard]] uint32_t GetFullScanCode() const;

private:
    const std::wstring chars_;
    const uint32_t virtualCode_ = 0;
    const uint32_t scanCode_ = 0;
    const bool isExtendedScanCode_ = false;
};

} // namespace smp
