#pragma once

#include <events/panel_event.h>

namespace smp
{

class PlaybackStopEvent : public PanelEvent
{
public:
    PlaybackStopEvent( smp::EventId id, play_control::t_stop_reason reason );

    std::unique_ptr<EventBase> Clone() override;

    play_control::t_stop_reason GetReason() const;

private:
    play_control::t_stop_reason reason_;
};

} // namespace smp
