#pragma once

#include <tasks/events/panel_event.h>

namespace smp
{

class LibraryItemEvent : public PanelEvent
{
public:
    [[nodiscard]] LibraryItemEvent( EventId id, not_null_shared<const metadb_handle_list> pHandles );

    [[nodiscard]] std::unique_ptr<EventBase> Clone() override;

    [[nodiscard]] const metadb_handle_list& GetHandles() const;

private:
    not_null_shared<const metadb_handle_list> pHandles_;
};

} // namespace smp
