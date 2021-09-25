// Based on https://searchfox.org/mozilla-central/source/dom/base/TimeoutManager.cpp

#include <stdafx.h>

#include "timeout_manager.h"

#include <events/event_js_executor.h>
#include <js_engine/js_container.h>
#include <js_utils/js_async_task.h>
#include <panel/js_panel_window.h>
#include <timeout/timeout_executor.h>

#include <qwr/final_action.h>

using namespace smp;

namespace
{

constexpr std::chrono::microseconds kTimeoutMaxConsecutiveCallbacks{ 4 };

}

namespace smp
{

TimeoutManager::TimeoutManager( std::shared_ptr<PanelTarget> pTarget )
    : pTarget_( pTarget )
    , timeoutStorage_( *this )
    , pExecutor_( std::make_shared<TimeoutExecutor>( *this, pTarget ) )
{
}

TimeoutManager::~TimeoutManager()
{
}

void TimeoutManager::Finalize()
{
    assert( pExecutor_ );
    pExecutor_->Shutdown();
}

void TimeoutManager::SetLoadingStatus( bool isLoading )
{
    if ( isLoading == isLoading_ )
    {
        return;
    }

    isLoading_ = isLoading;
    if ( isLoading_ )
    {
        StopAllTimeouts();
    }
    else
    {
        for ( const auto& pTimer: delayedTimeouts_ )
        {
            timeoutStorage_.Insert( pTimer );
        }
        delayedTimeouts_.clear();

        if ( const auto timeoutIt = timeoutStorage_.GetFirst();
             !timeoutStorage_.IsEnd( timeoutIt ) )
        {
            MaybeSchedule( ( *timeoutIt )->When() );
        }
    }
}

uint32_t TimeoutManager::SetInterval( uint32_t interval, std::unique_ptr<mozjs::JsAsyncTask> pJsTask )
{
    return CreateTimeout( interval, true, std::move( pJsTask ) );
}

uint32_t TimeoutManager::SetTimeout( uint32_t delay, std::unique_ptr<mozjs::JsAsyncTask> pJsTask )
{
    return CreateTimeout( delay, false, std::move( pJsTask ) );
}

void TimeoutManager::ClearTimeout( uint32_t timerId )
{
    auto timeoutIt = timeoutStorage_.Get( timerId );
    if ( timeoutStorage_.IsEnd( timeoutIt ) )
    {
        return;
    }

    bool deferredDeletion = false;
    const auto isFirstTimeout = ( timeoutIt == timeoutStorage_.GetFirst() );
    if ( auto& pTimeout = *timeoutIt;
         pTimeout->IsRunning() )
    {
        /*
            We're running from inside the timeout. Mark this
            timeout for deferred deletion by the code in
            RunTimeout()
        */

        pTimeout->DisableRepeat();
        deferredDeletion = true;
    }
    else
    {
        timeoutStorage_.Erase( timeoutIt );
    }

    // We don't need to reschedule the executor if any of the following are true:
    //  * If the we weren't cancelling the first timeout, then the executor's
    //    state doesn't need to change. It will only reflect the next soonest
    //    Timeout.
    //  * If we did cancel the first Timeout, but its currently running, then
    //    RunTimeout() will handle rescheduling the executor.
    if ( !isFirstTimeout || deferredDeletion )
    {
        return;
    }

    // Stop the executor and restart it at the next soonest deadline.
    pExecutor_->Cancel( false );
    if ( auto nextTimeoutIt = timeoutStorage_.GetFirst();
         !timeoutStorage_.IsEnd( nextTimeoutIt ) )
    {
        auto& pNextTimeout = *nextTimeoutIt;
        MaybeSchedule( pNextTimeout->When() );
    }
}

void TimeoutManager::StopAllTimeouts()
{
    pExecutor_->Cancel( true );
    timeoutStorage_.Clear();
}

void TimeoutManager::RunTimeout( const TimeStamp& now, const TimeStamp& targetDeadline )
{
    // Limit the overall time spent in RunTimeout() to reduce jank.
    const TimeDuration totalTimeLimit = kTimeoutMaxConsecutiveCallbacks;

    // Allow up to 25% of our total time budget to be used figuring out which
    // timers need to run. This is the initial loop in this method.
    const TimeDuration initialTimeLimit = totalTimeLimit / 4;

    // Ammortize overhead from from calling TimeStamp::Now() in the initial
    // loop, though, by only checking for an elapsed limit every N timeouts.
    const uint32_t kNumTimersPerInitialElapsedCheck = 100;

    // Start measuring elapsed time immediately. We won't potentially expire
    // the time budget until at least one timeout has run, though.
    TimeStamp nowSnapshot( now );
    TimeStamp start = nowSnapshot;

    uint32_t firingId = CreateFiringId();
    auto guard = qwr::final_action( [&] { DestroyFiringId( firingId ); } );

    // A native timer has gone off. See which of our timeouts need
    // servicing
    auto deadline = [&] {
        if ( targetDeadline > nowSnapshot )
        {
            // The OS timer fired early (which can happen due to the timers
            // having lower precision than TimeStamp does). Set |deadline| to
            // be the time when the OS timer *should* have fired so that any
            // timers that *should* have fired *will* be fired now.

            return targetDeadline;
        }
        else
        {
            return nowSnapshot;
        }
    }();

    std::optional<TimeStamp> nextDeadlineOpt;
    uint32_t numTimersToRun = 0;

    // The timeout list is kept in deadline order. Discover the latest timeout
    // whose deadline has expired. On some platforms, native timeout events fire
    // "early", but we handled that above by setting deadline to `targetDeadline`
    // if the timer fired early. So we can stop walking if we get to timeouts
    // whose When() is greater than deadline, since once that happens we know
    // nothing past that point is expired.

    for ( auto timeoutIt = timeoutStorage_.GetFirst(); !timeoutStorage_.IsEnd( timeoutIt ); timeoutIt = timeoutStorage_.GetNext( timeoutIt ) )
    {
        const auto& pTimeout = *timeoutIt;
        if ( pTimeout->When() > deadline )
        {
            nextDeadlineOpt = pTimeout->When();
            break;
        }

        if ( !IsValidFiringId( pTimeout->GetFiringId() ) )
        {
            // Mark any timeouts that are on the list to be fired with the
            // firing depth so that we can reentrantly run timeouts
            pTimeout->SetFiringId( firingId );

            ++numTimersToRun;

            // Run only a limited number of timers based on the configured maximum.
            if ( numTimersToRun % kNumTimersPerInitialElapsedCheck == 0 )
            {
                nowSnapshot = TimeStamp::clock::now();
                const auto elapsed = nowSnapshot - start;
                if ( elapsed >= initialTimeLimit )
                {
                    nextDeadlineOpt = pTimeout->When();
                    break;
                }
            }
        }
    }

    nowSnapshot = TimeStamp::clock::now();

    // Wherever we stopped in the timer list, schedule the executor to
    // run for the next unexpired deadline. Note, this *must* be done
    // before we start executing any script handlers. If one
    // of them spins the event loop the executor must already be scheduled
    // in order for timeouts to fire properly.
    if ( nextDeadlineOpt )
    {
        MaybeSchedule( *nextDeadlineOpt );
    }

    // Maybe the timeout that the event was fired for has been deleted
    // and there are no others timeouts with deadlines that make them
    // eligible for execution yet. Go away.
    if ( !numTimersToRun )
    {
        return;
    }

    // Now we need to search the normal and tracking timer list at the same
    // time to run the timers in the scheduled order.

    // We stop iterating each list when we go past the last expired timeout from
    // that list that we have observed above. That timeout will either be the
    // next item after the last timeout we looked at or nullptr if we have
    // exhausted the entire list while looking for the last expired timeout.

    // The next timeout to run. This is used to advance the loop, but
    // we cannot set it until we've run the current timeout, since
    // running the current timeout might remove the immediate next
    // timeout.
    TimeoutStorage::TimeoutIterator nextTimeoutIt;

    for ( auto timeoutIt = timeoutStorage_.GetFirst(); !timeoutStorage_.IsEnd( timeoutIt ); timeoutIt = nextTimeoutIt )
    {
        const auto pTimeout = *timeoutIt;

        nextTimeoutIt = timeoutStorage_.GetNext( timeoutIt );
        // We should only execute callbacks for the set of expired Timeout
        // objects we computed above.
        if ( pTimeout->GetFiringId() != firingId )
        {
            // If the FiringId does not match, but is still valid, then this is
            // a Timeout for another RunTimeout() on the call stack.
            // Just skip it.
            if ( IsValidFiringId( pTimeout->GetFiringId() ) )
            {
                continue;
            }

            // If, however, the FiringId is invalid then we have reached Timeout
            // objects beyond the list we calculated above. This can happen
            // if the Timeout just beyond our last expired Timeout is cancelled
            // by one of the callbacks we've just executed. In this case we
            // should just stop iterating. We're done.
            break;
        }

        // The timeout is on the list to run at this depth, go ahead and
        // process it.

        if ( isLoading_ )
        {
            // Any timeouts that would fire during a load will be deferred
            // until the load event occurs

            timeoutStorage_.Erase( timeoutIt );
            delayedTimeouts_.emplace_back( pTimeout );
        }
        else
        {
            // Save target in the local variable in case TimeoutManager is destroyed during timeout call
            const auto pTarget = pTarget_;
            if ( !pTarget->GetPanel() )
            {
                // Means that the panel was destroyed at some point.
                return;
            }

            // This timeout is good to run.
            pTimeout->SetRunningState( true );
            const auto has_succeeded = pTarget_->GetPanel()->ExecuteEvent_JsCode( *pTimeout->Task() );
            pTimeout->SetRunningState( false );
            if ( !has_succeeded || !pTarget->GetPanel() )
            {
                assert( timeoutStorage_.IsEmpty() );
                return;
            }

            // If we need to reschedule a setInterval() the delay should be
            // calculated based on when its callback started to execute. So
            // save off the last time before updating our "now" timestamp to
            // account for its callback execution time.
            const auto lastCallbackTime = nowSnapshot;
            nowSnapshot = TimeStamp::clock::now();

            // If we have a regular interval timer, we re-schedule the
            // timeout, accounting for clock drift.
            const auto needsReinsertion =
                RescheduleTimeout( *pTimeout, lastCallbackTime, nowSnapshot );

            // Running a timeout can cause another timeout to be deleted, so
            // we need to reset the pointer to the following timeout.
            nextTimeoutIt = timeoutStorage_.GetNext( timeoutIt );
            timeoutStorage_.Erase( timeoutIt );

            if ( needsReinsertion )
            {
                // Always re-insert into the normal time queue!
                timeoutStorage_.Insert( pTimeout );
            }
        }
        // Check to see if we have run out of time to execute timeout handlers.
        // If we've exceeded our time budget then terminate the loop immediately.
        const auto elapsed = nowSnapshot - start;
        if ( elapsed >= totalTimeLimit )
        {
            // We ran out of time. Make sure to schedule the executor to
            // run immediately for the next timer, if it exists.
            if ( !timeoutStorage_.IsEnd( nextTimeoutIt ) )
            {
                MaybeSchedule( ( *nextTimeoutIt )->When() );
            }
            break;
        }
    }
}

uint32_t TimeoutManager::CreateTimeout( uint32_t interval, bool isRepeated, std::unique_ptr<mozjs::JsAsyncTask> pJsTask )
{
    assert( pJsTask );

    const auto id = [&] {
        uint32_t id = curTimerId_++;
        while ( !timeoutStorage_.IsEnd( timeoutStorage_.Get( id ) ) )
        {
            id = curTimerId_++;
        }
        return id;
    }();

    auto pTimer = std::make_shared<Timeout>( id, std::chrono::milliseconds( interval ), isRepeated, std::move( pJsTask ) );
    pTimer->SetWhen( TimeStamp::clock::now(), pTimer->Interval() );

    MaybeSchedule( pTimer->When() );

    timeoutStorage_.Insert( pTimer );

    return id;
}

void TimeoutManager::MaybeSchedule( const TimeStamp& whenToTrigger )
{
    pExecutor_->MaybeSchedule( whenToTrigger );
}

bool TimeoutManager::RescheduleTimeout( Timeout& timeout, const TimeStamp& lastCallbackTime, const TimeStamp& currentNow )
{
    assert( lastCallbackTime <= currentNow );

    if ( !timeout.IsRepeated() )
    {
        return false;
    }

    // Automatically increase the nesting level when a setInterval()
    // is rescheduled just as if it was using a chained setTimeout().

    // TODO: nesting limits

    const auto firingTime = lastCallbackTime + timeout.Interval();
    auto delay = firingTime - currentNow;

    // And make sure delay is nonnegative; that might happen if the timer
    // thread is firing our timers somewhat early or if they're taking a long
    // time to run the callback.
    if ( delay < TimeDuration( 0 ) )
    {
        delay = TimeDuration( 0 );
    }

    timeout.SetWhen( currentNow, delay );

    MaybeSchedule( timeout.When() );

    return true;
}

uint32_t TimeoutManager::CreateFiringId()
{
    auto id = nextFiringId++;
    if ( nextFiringId == kInvalidFiringId )
    {
        ++nextFiringId;
    }

    activeFiringIds_.push_back( id );

    return id;
}

void TimeoutManager::DestroyFiringId( uint32_t id )
{
    assert( !activeFiringIds_.empty() );
    assert( activeFiringIds_.back() == id );
    activeFiringIds_.pop_back();
}

bool TimeoutManager::IsValidFiringId( uint32_t id ) const
{
    if ( id == kInvalidFiringId || activeFiringIds_.empty() )
    {
        return false;
    }

    const auto reverseRange = ranges::views::reverse( activeFiringIds_ );
    return ( ranges::find( reverseRange, id ) != ranges::end( reverseRange ) );
}

TimeoutManager::TimeoutStorage::TimeoutStorage( TimeoutManager& pParent )
    : pParent_( pParent )
{
}

TimeoutManager::TimeoutStorage::TimeoutIterator
TimeoutManager::TimeoutStorage::Get( uint32_t id )
{
    return ( idToIterator.contains( id ) ? idToIterator.at( id ) : schedule_.end() );
}

TimeoutManager::TimeoutStorage::TimeoutIterator
TimeoutManager::TimeoutStorage::GetFirst()
{
    return schedule_.begin();
}

smp::TimeoutManager::TimeoutStorage::TimeoutIterator
TimeoutManager::TimeoutStorage::GetLast()
{
    return ( schedule_.empty() ? schedule_.end() : --schedule_.end() );
}

TimeoutManager::TimeoutStorage::TimeoutIterator
TimeoutManager::TimeoutStorage::GetNext( const TimeoutIterator& it )
{
    return ( it == schedule_.end() ? it : std::next( it ) );
}

bool TimeoutManager::TimeoutStorage::IsEnd( const TimeoutIterator& it ) const
{
    return ( it == schedule_.end() );
}

void TimeoutManager::TimeoutStorage::Insert( std::shared_ptr<Timeout> pTimeout )
{
    assert( pTimeout );

    // Start at last timeout and go backwards. Stop if we see a Timeout with a
    // valid FiringId since those timers are currently being processed by
    // RunTimeout.  This optimizes for the common case of insertion at the end.

    auto reverseIt = ranges::find_if_not( schedule_ | ranges::views::reverse, [&]( const auto& pLocalTimeout ) {
        return ( pLocalTimeout->When() > pTimeout->When()
                 // Check the firing ID last since it will be invalid in the vast majority of cases.
                 && !pParent_.IsValidFiringId( pLocalTimeout->GetFiringId() ) );
    } );

    pTimeout->SetFiringId( kInvalidFiringId );
    auto insertedIt = schedule_.insert( reverseIt.base(), pTimeout );
    idToIterator.try_emplace( pTimeout->Id(), insertedIt );
}

void TimeoutManager::TimeoutStorage::Erase( TimeoutIterator it )
{
    idToIterator.erase( ( *it )->Id() );
    schedule_.erase( it );
}

bool TimeoutManager::TimeoutStorage::IsEmpty() const
{
    return schedule_.empty();
}

void TimeoutManager::TimeoutStorage::Clear()
{
    for ( auto& pTimer: schedule_ )
    {
        pTimer->MarkAsStopped();
    }
    idToIterator.clear();
    schedule_.clear();
}

} // namespace smp
