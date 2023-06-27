#include <stdafx.h>

#include "playlist_item_event.h"

namespace smp
{

PlaylistItemEvent::PlaylistItemEvent( EventId id, int32_t playlistIdx, uint32_t trackIndex )
    : PlaylistEvent( id, playlistIdx )
    , trackIndex_( trackIndex )
{
}

std::unique_ptr<smp::EventBase> PlaylistItemEvent::Clone()
{
    return std::make_unique<PlaylistItemEvent>( id_, GetPlaylistIndex(), trackIndex_ );
}

int32_t PlaylistItemEvent::GetItemIndex() const
{
    return trackIndex_;
}

} // namespace smp
