#pragma once

#include <tasks/events/panel_event.h>

namespace smp
{

class TrackEvent : public PanelEvent
{
public:
    [[nodiscard]] TrackEvent( EventId id, metadb_handle_list affectedTracks );

    [[nodiscard]] std::unique_ptr<EventBase> Clone() override;

    [[nodiscard]] metadb_handle_list GetAffectedTracks() const;

private:
    metadb_handle_list affectedTracks_;
};

} // namespace smp
