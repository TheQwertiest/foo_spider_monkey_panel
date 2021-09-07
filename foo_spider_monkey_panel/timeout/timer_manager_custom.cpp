// based on https://searchfox.org/mozilla-central/source/xpcom/threads/TimerThread.cpp

#include <stdafx.h>

#include "timer_manager_custom.h"

#include <events/event_dispatcher.h>
#include <events/event_timer.h>
#include <panel/js_panel_window.h>
#include <timeout/timer_custom.h>

#include <qwr/thread_helpers.h>

using namespace smp;

namespace smp
{

TimerManager_Custom::TimerManager_Custom()
{
    CreateThread();
}

void TimerManager_Custom::Finalize()
{
    StopThread();
}

TimerManager_Custom& TimerManager_Custom::Get()
{
    static TimerManager_Custom tm;
    return tm;
}

const TimeDuration& TimerManager_Custom::GetAllowedEarlyFiringTime()
{
    static constexpr TimeDuration earlyDelay{ std::chrono::microseconds( 10 ) };
    return earlyDelay;
}

std::unique_ptr<Timer_Custom> TimerManager_Custom::CreateTimer( std::shared_ptr<PanelTarget> pTarget )
{
    return std::unique_ptr<Timer_Custom>( new Timer_Custom( *this, pTarget ) );
}

void TimerManager_Custom::CreateThread()
{
    thread_ = std::make_unique<std::thread>( &TimerManager_Custom::ThreadMain, this );
    qwr::SetThreadName( *thread_, "SMP TimerManager" );
}

void TimerManager_Custom::StopThread()
{
    if ( !thread_ )
    {
        return;
    }

    {
        std::unique_lock<std::mutex> lock( threadMutex_ );
        isTimeToDie_ = true;
    }
    cv_.notify_all();

    if ( thread_->joinable() )
    {
        thread_->join();
    }

    thread_.reset();
    timers_.clear();
}

void TimerManager_Custom::ThreadMain()
{
    const auto hasWorkToDo = [&] {
        return ( !timers_.empty() || isTimeToDie_ );
    };

    while ( !isTimeToDie_ )
    {
        std::unique_lock sl( threadMutex_ );

        std::optional<TimeStamp> waitUntilOpt;
        while ( hasWorkToDo() )
        {
            RemoveLeadingCanceledTimersInternal();
            if ( timers_.empty() )
            {
                break;
            }

            assert( timers_.front() );
            auto pTimer = timers_.front()->Value();
            assert( pTimer );

            const auto now = TimeStamp::clock::now();
            if ( now < pTimer->When() - GetAllowedEarlyFiringTime() )
            {
                // clamp to ms
                auto diffInMs = std::chrono::duration_cast<std::chrono::milliseconds>( pTimer->When() - now );
                if ( !diffInMs.count() )
                { // round sub-millisecond waits to 1
                    diffInMs = std::chrono::milliseconds( 1 );
                }

                waitUntilOpt = now + diffInMs;

                break;
            }

            waitUntilOpt.reset();
            RemoveFirstTimerInternal();

            EventDispatcher::Get().PutEvent( pTimer->Target().GetHwnd(), std::make_unique<Event_Timer>( pTimer, pTimer->Generation() ) );
        }

        // spurious wake up guard has a HUUUUGE CPU overhead, hence we don't use it
        if ( waitUntilOpt )
        {
            cv_.wait_until( sl, *waitUntilOpt );
        }
        else
        {
            cv_.wait( sl );
        }
    }
}

void TimerManager_Custom::AddTimer( std::shared_ptr<Timer_Custom> pTimer )
{
    {
        std::unique_lock sl( threadMutex_ );

        const auto& pHolder = timers_.emplace_back( std::make_unique<TimerHolder>( pTimer ) );
        pTimer->SetHolder( timers_.back().get() );

        std::push_heap( timers_.begin(), timers_.end(), TimerSorter );
    }
    cv_.notify_all();
}

void TimerManager_Custom::RemoveTimer( std::shared_ptr<Timer_Custom> pTimer )
{
    {
        std::unique_lock sl( threadMutex_ );

        if ( !pTimer || !pTimer->Holder() )
        {
            return;
        }

        // Only mark as cancelled here, it will be removed later in the thread loop
        pTimer->Holder()->ResetValue();
    }
    cv_.notify_all();
}

void TimerManager_Custom::RemoveLeadingCanceledTimersInternal()
{
    // Move all canceled timers from the front of the list to
    // the back of the list using std::pop_heap().
    decltype( timers_ )::iterator sortedEnd;
    for ( sortedEnd = timers_.end(); sortedEnd != timers_.begin() && !timers_[0]->Value(); --sortedEnd )
    {
        std::pop_heap( timers_.begin(), sortedEnd, TimerSorter );
    }

    // If there were no canceled timers then we are done.
    if ( sortedEnd == timers_.end() )
    {
        return;
    }

    // Finally, remove the canceled timers from the back.
    timers_.resize( timers_.size() - std::distance( sortedEnd, timers_.end() ) );
}

void TimerManager_Custom::RemoveFirstTimerInternal()
{
    assert( !timers_.empty() );

    std::pop_heap( timers_.begin(), timers_.end(), TimerSorter );
    timers_.pop_back();
}

bool TimerManager_Custom::TimerSorter( const std::unique_ptr<TimerHolder>& a, const std::unique_ptr<TimerHolder>& b )
{
    // This is reversed because std::push_heap() sorts the "largest" to
    // the front of the heap.  We want that to be the earliest timer.
    return ( b->When() < a->When() );
}

} // namespace smp
