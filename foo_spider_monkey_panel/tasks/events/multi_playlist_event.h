#pragma once

#include <tasks/events/panel_event.h>

namespace smp
{

class MultiPlaylistEvent : public PanelEvent
{
public:
    [[nodiscard]] MultiPlaylistEvent( EventId id, const not_null_shared<const std::vector<uint32_t>> pPlaylistIndices );

    [[nodiscard]] std::unique_ptr<EventBase> Clone() override;

    [[nodiscard]] const std::vector<uint32_t>& GetPlaylistIndices() const;

private:
    not_null_shared<const std::vector<uint32_t>> pPlaylistIndices_;
};

} // namespace smp
