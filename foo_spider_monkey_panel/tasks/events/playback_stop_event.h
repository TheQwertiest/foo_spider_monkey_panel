#pragma once

#include <tasks/events/panel_event.h>

namespace smp
{

class PlaybackStopEvent : public PanelEvent
{
public:
    [[nodiscard]] PlaybackStopEvent( EventId id, play_control::t_stop_reason reason );

    [[nodiscard]] std::unique_ptr<EventBase> Clone() override;

    [[nodiscard]] play_control::t_stop_reason GetReason() const;

private:
    play_control::t_stop_reason reason_;
};

} // namespace smp
