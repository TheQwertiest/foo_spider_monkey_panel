#include <stdafx.h>

#include "playback_stop_event.h"

namespace smp
{

PlaybackStopEvent::PlaybackStopEvent( smp::EventId id, play_control::t_stop_reason reason )
    : JsCallbackEventNew( id )
    , reason_( reason )
{
}

std::unique_ptr<smp::EventBase> PlaybackStopEvent::Clone()
{
    return std::make_unique<PlaybackStopEvent>( id_, reason_ );
}

play_control::t_stop_reason PlaybackStopEvent::Reason() const
{
    return reason_;
}

} // namespace smp
