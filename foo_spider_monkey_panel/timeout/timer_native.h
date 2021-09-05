#pragma once

#include <timeout/time_types.h>

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

namespace smp
{

class PanelTarget;
class TimerManager_Native;

struct TimerNotifyTask_Native
{
    virtual ~TimerNotifyTask_Native() = default;
    virtual void Notify() = 0;
};

/// @brief Timer information holder.
///
/// Each SMP panel should hold no more than one timer. There are no technical limitations for this though.
class Timer_Native : public std::enable_shared_from_this<Timer_Native>
{
    friend class TimerManager_Native;

public:
    ~Timer_Native() = default;

    void Start( TimerNotifyTask_Native& task, const TimeDuration& delay );
    void Cancel( bool waitForDestruction );

    void Fire( uint64_t generation );

    [[nodiscard]] PanelTarget& Target() const;
    [[nodiscard]] const TimeStamp& When() const;
    [[nodiscard]] uint64_t Generation() const;

private:
    Timer_Native( TimerManager_Native& pParent, std::shared_ptr<PanelTarget> pTarget );

    static VOID CALLBACK TimerProc( PVOID lpParameter, BOOLEAN TimerOrWaitFired );

private:
    TimerManager_Native& pParent_;
    std::shared_ptr<PanelTarget> pTarget_;
    HANDLE hTimer_ = nullptr;

    TimerNotifyTask_Native* pTask_ = nullptr;
    TimeStamp executeAt_{};
    int64_t generation_ = 0;
};

} // namespace smp
