#include <stdafx.h>

#include "playlist_multi_item_event.h"

namespace smp
{

PlaylistMultiItemEvent::PlaylistMultiItemEvent( EventId id, int32_t playlistIdx, not_null_shared<const std::vector<uint32_t>> pItemIndices )
    : PlaylistEvent( id, playlistIdx )
    , pItemIndices_( pItemIndices )
{
}

std::unique_ptr<smp::EventBase> PlaylistMultiItemEvent::Clone()
{
    return std::make_unique<PlaylistMultiItemEvent>( id_, GetPlaylistIndex(), pItemIndices_ );
}

const std::vector<uint32_t>& PlaylistMultiItemEvent::GetItemIndices() const
{
    return *pItemIndices_;
}

} // namespace smp
