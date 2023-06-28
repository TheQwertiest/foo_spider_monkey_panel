#include <stdafx.h>

#include "playback_queue_event.h"

namespace smp
{

PlaybackQueueEvent::PlaybackQueueEvent( EventId id, playback_queue_callback::t_change_origin origin )
    : PanelEvent( id )
    , origin_( origin )
{
}

std::unique_ptr<EventBase> PlaybackQueueEvent::Clone()
{
    return std::make_unique<PlaybackQueueEvent>( id_, origin_ );
}

playback_queue_callback::t_change_origin PlaybackQueueEvent::GetOrigin() const
{
    return origin_;
}

} // namespace smp
