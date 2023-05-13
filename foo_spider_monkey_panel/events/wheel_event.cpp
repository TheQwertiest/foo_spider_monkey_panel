#include <stdafx.h>

#include "wheel_event.h"

namespace smp
{

WheelEvent::WheelEvent( EventId id,
                        MouseKeyFlag button,
                        int32_t x,
                        int32_t y,
                        int32_t delta,
                        WheelDirection direction,
                        WheelMode mode )
    : MouseEvent( id, button, x, y )
    , delta_( delta )
    , direction_( direction )
    , mode_( mode )
{
}

int32_t WheelEvent::GetDelta() const
{
    return delta_;
}

WheelEvent::WheelDirection WheelEvent::GetDirection() const
{
    return direction_;
}

WheelEvent::WheelMode WheelEvent::GetMode() const
{
    return mode_;
}

} // namespace smp
