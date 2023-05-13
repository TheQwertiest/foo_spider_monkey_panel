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
                                 const qwr::u8string& key,
                                 uint32_t code );

    // TODO: save all other data
    const qwr::u8string& GetKey() const;

    uint32_t GetCode() const;

private:
    const qwr::u8string key_;
    uint32_t code_;
};

} // namespace smp
