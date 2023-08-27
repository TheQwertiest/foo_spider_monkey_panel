#pragma once

#include <tasks/events/panel_event.h>

namespace smp
{

class PlaylistEvent : public PanelEvent
{
public:
    [[nodiscard]] PlaylistEvent( EventId id, int32_t playlistIndex );

    [[nodiscard]] std::unique_ptr<EventBase> Clone() override;

    [[nodiscard]] int32_t GetPlaylistIndex() const;

private:
    const int32_t playlistIndex_;
};

} // namespace smp
