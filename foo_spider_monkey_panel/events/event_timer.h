#pragma once

#include <events/event.h>

#include <memory>

namespace smp
{

class Timer;

class Event_Timer
    : public EventBase
{
public:
    Event_Timer( std::shared_ptr<Timer> pTimer, uint64_t generation );

    void Run() override;

private:
    std::shared_ptr<Timer> pTimer_;
    uint64_t generation_;
};

} // namespace smp
