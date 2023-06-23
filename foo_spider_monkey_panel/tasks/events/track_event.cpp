#include <stdafx.h>

#include "track_event.h"

namespace smp
{

TrackEvent::TrackEvent( smp::EventId id, metadb_handle_list affectedTracks )
    : PanelEvent( id )
    , affectedTracks_( affectedTracks )
{
}

std::unique_ptr<smp::EventBase> TrackEvent::Clone()
{
    return std::make_unique<TrackEvent>( id_, affectedTracks_ );
}

metadb_handle_list TrackEvent::GetAffectedTracks() const
{
    return affectedTracks_;
}

} // namespace smp
