#pragma once

#include <events/js_callback_event.h>

namespace mozjs
{

class JsContainer;

}

namespace smp
{

class PlaybackStopEvent : public smp::JsCallbackEventNew
{
public:
    PlaybackStopEvent( smp::EventId id, play_control::t_stop_reason reason );

    std::unique_ptr<EventBase> Clone() override;

    play_control::t_stop_reason Reason() const;

private:
    play_control::t_stop_reason reason_;
};

} // namespace smp
