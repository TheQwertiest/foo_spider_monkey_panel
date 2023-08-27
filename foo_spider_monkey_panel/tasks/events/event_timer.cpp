#include <stdafx.h>

#include "event_timer.h"

#include <cassert>

namespace smp
{

Event_Timer::Event_Timer( std::shared_ptr<ITimer> pTimer, uint64_t generation )
    : EventBase( EventId::kTimer )
    , pTimer_( pTimer )
    , generation_( generation )
{
    assert( pTimer_ );
}

void Event_Timer::Run()
{
    pTimer_->Fire( generation_ );
}

} // namespace smp
