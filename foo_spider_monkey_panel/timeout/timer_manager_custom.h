#pragma once

#include <timeout/time_types.h>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

namespace smp
{

class PanelTarget;
class Timer_Custom;
class TimerHolder;

/// @brief Actual timer implementation.
class TimerManager_Custom
{
public:
    static [[nodiscard]] TimerManager_Custom& Get();

    void Finalize();

    [[nodiscard]] static const TimeDuration& GetAllowedEarlyFiringTime();
    [[nodiscard]] std::unique_ptr<Timer_Custom> CreateTimer( std::shared_ptr<PanelTarget> pTarget );

public:
    void AddTimer( std::shared_ptr<Timer_Custom> pTimer );
    void RemoveTimer( std::shared_ptr<Timer_Custom> pTimer );

private:
    TimerManager_Custom();

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

} // namespace smp
