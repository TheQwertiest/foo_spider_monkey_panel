#include <stdafx.h>

#include "multi_playlist_event.h"

namespace smp
{

MultiPlaylistEvent::MultiPlaylistEvent( smp::EventId id, const not_null_shared<const std::vector<uint32_t>> pPlaylistIndices )
    : PanelEvent( id )
    , pPlaylistIndices_( pPlaylistIndices )
{
}

std::unique_ptr<smp::EventBase> MultiPlaylistEvent::Clone()
{
    return std::make_unique<MultiPlaylistEvent>( id_, pPlaylistIndices_ );
}

const std::vector<uint32_t>& MultiPlaylistEvent::GetPlaylistIndices() const
{
    return *pPlaylistIndices_;
}

} // namespace smp
