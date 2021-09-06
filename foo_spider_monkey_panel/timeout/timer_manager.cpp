// based on https://searchfox.org/mozilla-central/source/xpcom/threads/TimerThread.cpp

#include <stdafx.h>

#include "timer_manager.h"

#include <events/event_manager.h>
#include <events/event_timer.h>
#include <panel/js_panel_window.h>

#include <qwr/thread_helpers.h>

using namespace smp;

namespace smp
{

TimerManager::TimerManager()
{
    CreateThread();
}

void TimerManager::Finalize()
{
    StopThread();
}

TimerManager& TimerManager::Get()
{
    static TimerManager tm;
    return tm;
}

const TimeDuration& TimerManager::GetAllowedEarlyFiringTime()
{
    static constexpr TimeDuration earlyDelay{ std::chrono::microseconds( 10 ) };
    return earlyDelay;
}

std::unique_ptr<Timer> TimerManager::CreateTimer( std::shared_ptr<PanelTarget> pTarget )
{
    return std::unique_ptr<Timer>( new Timer( *this, pTarget ) );
}

void TimerManager::CreateThread()
{
    thread_ = std::make_unique<std::thread>( &TimerManager::ThreadMain, this );
    qwr::SetThreadName( *thread_, "SMP TimerManager" );
}

void TimerManager::StopThread()
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

void TimerManager::ThreadMain()
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
                auto diffInMs = std::chrono::duration_cast<std::chrono::milliseconds>( pTimer->When() - TimeStamp::clock::now() );
                if ( !diffInMs.count() )
                { // round sub-millisecond waits to 1
                    diffInMs = std::chrono::milliseconds( 1 );
                }

                waitUntilOpt = now + diffInMs;

                break;
            }

            RemoveFirstTimerInternal();

            // TODO: replace HWND in PutEvent with target
            auto pPanel = pTimer->Target().GetPanel();
            if ( pPanel )
            {
                EventManager::Get().PutEvent( pPanel->GetHWND(), std::make_unique<Event_Timer>( pTimer, pTimer->Generation() ) );
            }
        }

        // we don't care about wake ups here
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

void TimerManager::AddTimer( std::shared_ptr<Timer> pTimer )
{
    {
        std::unique_lock sl( threadMutex_ );

        timers_.emplace_back( std::make_unique<TimerHolder>( pTimer ) );
        pTimer->SetHolder( timers_.back().get() );

        std::push_heap( timers_.begin(), timers_.end(), TimerSorter );
    }
    cv_.notify_all();
}

void TimerManager::RemoveTimer( std::shared_ptr<Timer> pTimer )
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

void TimerManager::RemoveLeadingCanceledTimersInternal()
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

void TimerManager::RemoveFirstTimerInternal()
{
    assert( !timers_.empty() );

    std::pop_heap( timers_.begin(), timers_.end(), TimerSorter );
    timers_.pop_back();
}

bool TimerManager::TimerSorter( const std::unique_ptr<TimerHolder>& a, const std::unique_ptr<TimerHolder>& b )
{
    // This is reversed because std::push_heap() sorts the "largest" to
    // the front of the heap.  We want that to be the earliest timer.
    return ( b->When() < a->When() );
}

TimerHolder::TimerHolder( std::shared_ptr<Timer> pTimer )
    : pTimer_( pTimer )
    , executeAt_( pTimer_->When() )
{
}

TimerHolder::~TimerHolder()
{
    ResetValue();
}

void TimerHolder::ResetValue()
{
    if ( pTimer_ )
    {
        pTimer_->SetHolder( nullptr );
        pTimer_.reset();
    }
}

std::shared_ptr<Timer> TimerHolder::Value() const
{
    return pTimer_;
}

const TimeStamp& TimerHolder::When() const
{
    return executeAt_;
}

Timer::Timer( TimerManager& pParent, std::shared_ptr<PanelTarget> pTarget )
    : pParent_( pParent )
    , pTarget_( pTarget )
{
}

void Timer::Start( TimerNotifyTask& task, const TimeDuration& delay )
{
    assert( core_api::is_main_thread() );

    std::scoped_lock sl( mutex_ );

    auto self = shared_from_this();

    pParent_.RemoveTimer( self );

    pTask_ = &task;
    ++generation_;

    executeAt_ = TimeStamp::clock::now() + delay;

    pParent_.AddTimer( self );
}

void Timer::Fire( uint64_t generation )
{
    // Save self, since it can be destroyed in `Notify` callback
    auto selfSaver = shared_from_this();
    decltype( pTask_ ) pTask = nullptr;

    {
        // Don't fire callbacks when the mutex is locked.
        // If some other thread Cancels after this, they're just too late.
        std::scoped_lock sl( mutex_ );
        if ( generation != generation_ )
        {
            return;
        }

        pTask = pTask_;
    }

    if ( pTask )
    {
        pTask->Notify();
    }

    {
        std::scoped_lock sl( mutex_ );
        if ( generation == generation_ )
        {
            pTask_ = nullptr;
        }
    }
}

void Timer::Cancel( bool /*waitForDestruction*/ )
{
    std::scoped_lock sl( mutex_ );

    auto self = shared_from_this();

    pParent_.RemoveTimer( self );

    pTask_ = nullptr;
    ++generation_;
}

void Timer::SetHolder( TimerHolder* pHolder )
{
    pHolder_ = pHolder;
}

PanelTarget& Timer::Target() const
{
    assert( pTarget_ );
    return *pTarget_;
}

const TimeStamp& Timer::When() const
{
    return executeAt_;
}

uint64_t Timer::Generation() const
{
    return generation_;
}

TimerHolder* Timer::Holder() const
{
    return pHolder_;
}

} // namespace smp
