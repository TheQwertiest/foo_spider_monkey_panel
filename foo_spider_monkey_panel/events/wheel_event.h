#pragma once

#include <events/mouse_event.h>

namespace smp
{

class WheelEvent : public MouseEvent
{
public:
    enum class WheelDirection
    {
        kHorizontal,
        kVertical,
    };
    enum class WheelMode
    {
        kPixel, // currently unused
        kLine,
        kPage
    };

public:
    [[nodiscard]] WheelEvent( EventId id,
                              MouseKeyFlag button,
                              int32_t x,
                              int32_t y,
                              int32_t delta,
                              WheelDirection direction,
                              WheelMode mode );

    [[nodiscard]] int32_t GetDelta() const;
    [[nodiscard]] WheelDirection GetDirection() const;
    [[nodiscard]] WheelMode GetMode() const;

private:
    int32_t delta_ = 0;
    WheelDirection direction_ = WheelDirection::kVertical;
    WheelMode mode_ = WheelMode::kLine;
};

} // namespace smp
