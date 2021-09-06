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

struct TimerNotifyTask
{
    virtual ~TimerNotifyTask() = default;
    virtual void Notify() = 0;
};

class Timer;
class TimerHolder;

/// @brief Actual timer implementation.
class TimerManager
{
public:
    static TimerManager& Get();

    void Finalize();

    [[nodiscard]] static const TimeDuration& GetAllowedEarlyFiringTime();
    [[nodiscard]] std::unique_ptr<Timer> CreateTimer( std::shared_ptr<PanelTarget> pTarget );

public:
    void AddTimer( std::shared_ptr<Timer> pTimer );
    void RemoveTimer( std::shared_ptr<Timer> pTimer );

private:
    TimerManager();

    void CreateThread();
    void StopThread();

    void ThreadMain();

    void RemoveLeadingCanceledTimersInternal();
    void RemoveFirstTimerInternal();

    static bool TimerSorter( const std::unique_ptr<TimerHolder>& a, const std::unique_ptr<TimerHolder>& b );

private:
    std::atomic_bool isTimeToDie_{ false };

    std::unique_ptr<std::thread> thread_;
    std::mutex threadMutex_;

    std::condition_variable cv_;
    std::vector<std::unique_ptr<TimerHolder>> timers_;
};

class TimerHolder
{
public:
    TimerHolder( std::shared_ptr<Timer> pTimer );
    ~TimerHolder();

    void ResetValue();

    std::shared_ptr<Timer> Value() const;
    const TimeStamp& When() const;

private:
    std::shared_ptr<Timer> pTimer_;
    TimeStamp executeAt_;
};

/// @brief Timer information holder.
///
/// Each SMP panel should hold no more than one timer. There are no technical limitations for this though.
class Timer : public std::enable_shared_from_this<Timer>
{
    friend class TimerManager;

public:
    ~Timer() = default;

    void Start( TimerNotifyTask& task, const TimeDuration& delay );
    void Cancel( bool waitForDestruction );

    void SetHolder( TimerHolder* pHolder );

    void Fire( uint64_t generation );

    [[nodiscard]] PanelTarget& Target() const;
    [[nodiscard]] const TimeStamp& When() const;
    [[nodiscard]] uint64_t Generation() const;
    [[nodiscard]] TimerHolder* Holder() const;

private:
    Timer( TimerManager& pParent, std::shared_ptr<PanelTarget> pTarget );

private:
    TimerManager& pParent_;
    std::shared_ptr<PanelTarget> pTarget_;
    TimerHolder* pHolder_ = nullptr;

    mutable std::mutex mutex_;
    TimerNotifyTask* pTask_ = nullptr;
    TimeStamp executeAt_{};
    int64_t generation_ = 0;
};

} // namespace smp
