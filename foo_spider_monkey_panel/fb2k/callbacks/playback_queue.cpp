#include <stdafx.h>

#include <tasks/dispatcher/event_dispatcher.h>
#include <tasks/events/playback_queue_event.h>

using namespace smp;

namespace
{

class PlaybackQueueCallbackImpl : public playback_queue_callback
{
public:
    void on_changed( t_change_origin p_origin ) final;
};

} // namespace

namespace
{

void PlaybackQueueCallbackImpl::on_changed( t_change_origin p_origin )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<PlaybackQueueEvent>( EventId::kNew_FbPlaybackQueueChanged, p_origin ) );
}

} // namespace

namespace
{

FB2K_SERVICE_FACTORY( PlaybackQueueCallbackImpl );

} // namespace
