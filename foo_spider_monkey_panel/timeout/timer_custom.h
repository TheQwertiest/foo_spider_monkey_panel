#pragma once

#include <timeout/time_types.h>
#include <timeout/timer_interface_fwd.h>

#include <memory>
#include <mutex>

namespace smp
{

class TimerHolder
{
public:
    TimerHolder( std::shared_ptr<Timer_Custom> pTimer );
    ~TimerHolder();

    void ResetValue();

    std::shared_ptr<Timer_Custom> Value() const;
    const TimeStamp& When() const;

private:
    std::shared_ptr<Timer_Custom> pTimer_;
    TimeStamp executeAt_;
};

/// @brief Timer information holder.
///
/// Each SMP panel should hold no more than one timer. There are no technical limitations for this though.
class Timer_Custom final
    : public ITimer
    , public std::enable_shared_from_this<Timer_Custom>
{
    friend class TimerManager_Custom;

public:
    ~Timer_Custom() = default;

    void Start( TimerNotifyTask& task, const TimeStamp& when );
    void Cancel( bool waitForDestruction );

    void SetHolder( TimerHolder* pHolder );

    void Fire( uint64_t generation );

    [[nodiscard]] PanelTarget& Target() const;
    [[nodiscard]] const TimeStamp& When() const;
    [[nodiscard]] uint64_t Generation() const;
    [[nodiscard]] TimerHolder* Holder() const;

private:
    Timer_Custom( TimerManager_Custom& pParent, std::shared_ptr<PanelTarget> pTarget );

private:
    TimerManager_Custom& pParent_;
    std::shared_ptr<PanelTarget> pTarget_;
    TimerHolder* pHolder_ = nullptr;

    mutable std::mutex mutex_;
    TimerNotifyTask* pTask_ = nullptr;
    TimeStamp executeAt_{};
    int64_t generation_ = 0;
};

} // namespace smp
