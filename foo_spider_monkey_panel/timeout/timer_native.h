#pragma once

#include <panel/panel_fwd.h>
#include <timeout/time_types.h>
#include <timeout/timer_interface_fwd.h>

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

namespace smp
{

class TimerManager_Native;

/// @brief Timer information holder.
///
/// Each SMP panel should hold no more than one timer. There are no technical limitations for this though.
class Timer_Native final
    : public ITimer
    , public std::enable_shared_from_this<Timer_Native>
{
    friend class TimerManager_Native;

public:
    ~Timer_Native() = default;

    void Start( TimerNotifyTask& task, const TimeStamp& when );
    void Cancel( bool waitForDestruction );

    void Fire( uint64_t generation );

    [[nodiscard]] not_null_shared<panel::PanelAccessor> Target() const;
    [[nodiscard]] const TimeStamp& When() const;
    [[nodiscard]] uint64_t Generation() const;

private:
    Timer_Native( TimerManager_Native& pParent, not_null_shared<panel::PanelAccessor> pTarget );

    static VOID CALLBACK TimerProc( PVOID lpParameter, BOOLEAN TimerOrWaitFired );

private:
    TimerManager_Native& pParent_;
    not_null_shared<panel::PanelAccessor> pTarget_;
    HANDLE hTimer_ = nullptr;

    TimerNotifyTask* pTask_ = nullptr;
    TimeStamp executeAt_{};
    int64_t generation_ = 0;
};

} // namespace smp
