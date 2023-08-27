#pragma once

#include <tasks/events/panel_event.h>

namespace smp
{

class PlaybackQueueEvent : public PanelEvent
{
public:
    [[nodiscard]] PlaybackQueueEvent( EventId id, playback_queue_callback::t_change_origin origin );

    [[nodiscard]] std::unique_ptr<EventBase> Clone() override;

    [[nodiscard]] playback_queue_callback::t_change_origin GetOrigin() const;

private:
    const playback_queue_callback::t_change_origin origin_;
};

} // namespace smp
