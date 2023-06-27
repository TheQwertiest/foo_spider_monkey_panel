#pragma once

#include <tasks/events/playlist_event.h>

namespace smp
{

class PlaylistItemEvent : public PlaylistEvent
{
public:
    [[nodiscard]] PlaylistItemEvent( EventId id, int32_t playlistIdx, uint32_t trackIndex );

    [[nodiscard]] std::unique_ptr<EventBase> Clone() override;

    [[nodiscard]] int32_t GetItemIndex() const;

private:
    const int32_t trackIndex_;
};

} // namespace smp
