#pragma once

#include <events/event.h>
#include <timeout/timeout.h>
#include <timeout/timer_interface.h>

namespace smp
{

class TimeoutManager;

class TimeoutExecutor
    : public Runnable
    , public TimerNotifyTask
    , public std::enable_shared_from_this<TimeoutExecutor>
{
public:
    TimeoutExecutor( TimeoutManager& pParent, std::shared_ptr<PanelTarget> pTarget );
    ~TimeoutExecutor();

    void Shutdown();
    void Cancel( bool waitForDestruction );

    void MaybeSchedule( const TimeStamp& targetDeadline );

    // Runnable
    void Run() override;

    // TimerNotifyTask
    void Notify() override;

private:
    void Schedule( const TimeStamp& targetDeadline );
    void MaybeReschedule( const TimeStamp& targetDeadline );

    void ScheduleImmediate( const TimeStamp& targetDeadline, const TimeStamp& now );
    void ScheduleDelayed( const TimeStamp& targetDeadline, const TimeStamp& now );

    void MaybeExecute();

private:
    // The TimeoutExecutor is repeatedly scheduled by the TimeoutManager
    // to fire for the next soonest Timeout.  Since the executor is re-used
    // it needs to handle switching between a few states.
    enum class Mode
    {
        // None indicates the executor is idle.  It may be scheduled or shutdown.
        None,
        // Immediate means the executor is scheduled to run a Timeout with a
        // deadline that has already expired.
        Immediate,
        // Delayed means the executor is scheduled to run a Timeout with a
        // deadline in the future.
        Delayed,
        // Shutdown means the TimeoutManager has been destroyed.  Once this
        // state is reached the executor cannot be scheduled again.  If the
        // executor is already dispatched as a runnable or held by a timer then
        // we may still get a Run()/Notify() call which will be ignored.
        Shutdown
    };

    TimeoutManager& pParent_;
    std::shared_ptr<PanelTarget> pTarget_;

    std::shared_ptr<ITimer> pTimer_;
    Mode mode_ = Mode::None;
    std::optional<TimeStamp> deadlineOpt_;

    bool usedCustomTimerEngine_ = false;
};

} // namespace smp
