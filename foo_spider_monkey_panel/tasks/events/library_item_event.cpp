#include <stdafx.h>

#include "library_item_event.h"

namespace smp
{

LibraryItemEvent::LibraryItemEvent( smp::EventId id, not_null_shared<const metadb_handle_list> pHandles )
    : PanelEvent( id )
    , pHandles_( pHandles )
{
}

std::unique_ptr<smp::EventBase> LibraryItemEvent::Clone()
{
    return std::make_unique<LibraryItemEvent>( id_, pHandles_ );
}

const metadb_handle_list& LibraryItemEvent::GetHandles() const
{
    return *pHandles_;
}

} // namespace smp
