// Based on https://searchfox.org/mozilla-central/source/dom/base/TimeoutExecutor.cpp

#include <stdafx.h>

#include "timeout_executor.h"

#include <events/event_manager.h>
#include <panel/js_panel_window.h>
#include <timeout/timeout_manager.h>

namespace smp
{

TimeoutExecutor::TimeoutExecutor( TimeoutManager& pParent, std::shared_ptr<PanelTarget> pTarget )
    : pParent_( pParent )
    , pTarget_( pTarget )
{
}

TimeoutExecutor::~TimeoutExecutor()
{
    // The TimeoutManager should keep the Executor alive until its destroyed,
    // and then call Shutdown() explicitly.
    assert( mode_ == Mode::Shutdown );
    assert( !pTimer_ );
}

void TimeoutExecutor::Shutdown()
{
    if ( pTimer_ )
    {
        pTimer_->Cancel();
        pTimer_ = nullptr;
    }

    mode_ = Mode::Shutdown;
    deadlineOpt_.reset();
}

void TimeoutExecutor::Cancel()
{
    if ( pTimer_ )
    {
        pTimer_->Cancel();
    }

    mode_ = Mode::None;
    deadlineOpt_.reset();
}

void TimeoutExecutor::MaybeSchedule( const TimeStamp& targetDeadline )
{
    if ( mode_ == Mode::Shutdown )
    {
        return;
    }

    if ( mode_ == Mode::Immediate || mode_ == Mode::Delayed )
    {
        return MaybeReschedule( targetDeadline );
    }

    return Schedule( targetDeadline );
}

void TimeoutExecutor::Run()
{
    // If the executor is canceled and then rescheduled its possible to get
    // spurious executions here.  Ignore these unless our current mode matches.
    if ( mode_ == Mode::Immediate )
    {
        MaybeExecute();
    }
}

void TimeoutExecutor::Notify()
{
    // If the executor is canceled and then rescheduled its possible to get
    // spurious executions here.  Ignore these unless our current mode matches.
    if ( mode_ == Mode::Delayed )
    {
        MaybeExecute();
    }
}

void TimeoutExecutor::Schedule( const TimeStamp& targetDeadline )
{
    const auto now( TimeStamp::clock::now() );

    if ( targetDeadline <= ( now + TimerManager::GetAllowedEarlyFiringTime() ) )
    {
        return ScheduleImmediate( targetDeadline, now );
    }

    return ScheduleDelayed( targetDeadline, now );
}

void TimeoutExecutor::MaybeReschedule( const TimeStamp& targetDeadline )
{
    assert( deadlineOpt_ );
    assert( mode_ == Mode::Immediate || mode_ == Mode::Delayed );

    if ( targetDeadline >= *deadlineOpt_ )
    {
        return;
    }

    if ( mode_ == Mode::Immediate )
    {
        // Don't reduce the deadline here as we want to execute the
        // timer we originally scheduled even if its a few microseconds
        // in the future.
        return;
    }

    Cancel();
    Schedule( targetDeadline );
}

void TimeoutExecutor::ScheduleImmediate( const TimeStamp& targetDeadline, const TimeStamp& now )
{
    assert( !deadlineOpt_ );
    assert( mode_ == Mode::None );
    assert( targetDeadline <= ( now + TimerManager::GetAllowedEarlyFiringTime() ) );

    // TODO: replace HWND in PutEvent with target
    auto pPanel = pTarget_->GetPanel();
    if ( !pPanel )
    {
        assert( false );
        return;
    }

    EventManager::Get().PutRunnable( pPanel->GetHWND(), shared_from_this() );

    mode_ = Mode::Immediate;
    deadlineOpt_ = targetDeadline;
}

void TimeoutExecutor::ScheduleDelayed( const TimeStamp& targetDeadline, const TimeStamp& now )
{
    assert( !deadlineOpt_ );
    assert( mode_ == Mode::None );
    assert( targetDeadline > ( now + TimerManager::GetAllowedEarlyFiringTime() ) );

    if ( !pTimer_ )
    {
        pTimer_ = TimerManager::Get().CreateTimer( pTarget_ );
        // Re-evaluate if we should have scheduled this immediately
        if ( targetDeadline <= ( now + TimerManager::GetAllowedEarlyFiringTime() ) )
        {
            return ScheduleImmediate( targetDeadline, now );
        }
    }
    else
    {
        // Always call Cancel() in case we are re-using a timer.
        pTimer_->Cancel();
    }

    // Calculate the delay based on the deadline and current time.
    TimeDuration delay = targetDeadline - now;

    // Note, we cannot use timers that take
    // integer milliseconds. We need higher precision. Consider this
    // situation:
    //
    // 1. setTimeout(f, 1);
    // 2. do some work for 500us
    // 3. setTimeout(g, 1);
    //
    // This should fire f() and g() 500us apart.
    pTimer_->Start( *this, delay );

    mode_ = Mode::Delayed;
    deadlineOpt_ = targetDeadline;
}

void TimeoutExecutor::MaybeExecute()
{
    assert( mode_ != Mode::Shutdown && mode_ != Mode::None );
    assert( deadlineOpt_ );

    TimeStamp deadline( *deadlineOpt_ );

    // Sometimes timer or canceled timers will fire too early.  If this
    // happens then just cap our deadline to our maximum time in the future
    // and proceed.  If there are no timers ready we will get rescheduled
    // by TimeoutManager.
    const auto now = TimeStamp::clock::now();
    const auto limit = now + TimerManager::GetAllowedEarlyFiringTime();
    if ( deadline > limit )
    {
        deadline = limit;
    }

    Cancel();

    pParent_.RunTimeout( now, deadline );
}

} // namespace smp
