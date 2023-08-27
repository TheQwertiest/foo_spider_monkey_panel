#pragma once

#include <tasks/events/playlist_event.h>

namespace smp
{

class PlaylistMultiItemEvent : public PlaylistEvent
{
public:
    [[nodiscard]] PlaylistMultiItemEvent( EventId id, int32_t playlistIdx, not_null_shared<const std::vector<uint32_t>> pItemIndices );

    [[nodiscard]] std::unique_ptr<EventBase> Clone() override;

    [[nodiscard]] const std::vector<uint32_t>& GetItemIndices() const;

private:
    not_null_shared<const std::vector<uint32_t>> pItemIndices_;
};

} // namespace smp
