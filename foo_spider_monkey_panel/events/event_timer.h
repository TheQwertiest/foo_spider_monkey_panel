#pragma once

#include <events/event.h>

#include <memory>

namespace smp
{

class Timer_Native;

class Event_Timer
    : public EventBase
{
public:
    Event_Timer( std::shared_ptr<Timer_Native> pTimer, uint64_t generation );

    void Run() override;

private:
    std::shared_ptr<Timer_Native> pTimer_;
    uint64_t generation_;
};

} // namespace smp
