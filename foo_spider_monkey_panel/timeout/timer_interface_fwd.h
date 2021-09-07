#pragma once

#include <timeout/time_types.h>

namespace smp
{

class PanelTarget;

class Timer_Native;
class TimerManager_Native;

class Timer_Custom;
class TimerManager_Custom;

struct TimerNotifyTask
{
    virtual ~TimerNotifyTask() = default;
    virtual void Notify() = 0;
};

class ITimer
{
public:
    virtual ~ITimer() = default;

    virtual void Start( TimerNotifyTask& task, const TimeStamp& when ) = 0;
    virtual void Cancel( bool waitForDestruction ) = 0;

    virtual void Fire( uint64_t generation ) = 0;

    virtual [[nodiscard]] PanelTarget& Target() const = 0;
    virtual [[nodiscard]] const TimeStamp& When() const = 0;
    virtual [[nodiscard]] uint64_t Generation() const = 0;
};

} // namespace smp
