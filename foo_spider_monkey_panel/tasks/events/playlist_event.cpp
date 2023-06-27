#include <stdafx.h>

#include "playlist_event.h"

namespace smp
{

PlaylistEvent::PlaylistEvent( smp::EventId id, int32_t playlistIndex )
    : PanelEvent( id )
    , playlistIndex_( playlistIndex )
{
}

std::unique_ptr<smp::EventBase> PlaylistEvent::Clone()
{
    return std::make_unique<PlaylistEvent>( id_, playlistIndex_ );
}

int32_t PlaylistEvent::GetPlaylistIndex() const
{
    return playlistIndex_;
}

} // namespace smp
