#pragma once

#include <timeout/time_types.h>

#include <chrono>
#include <memory>

namespace smp
{

class PanelTarget;
class Timer_Native;

class TimerManager_Native
{
public:
    static [[nodiscard]] TimerManager_Native& Get();

    void Finalize();

    [[nodiscard]] static const TimeDuration& GetAllowedEarlyFiringTime();
    [[nodiscard]] std::unique_ptr<Timer_Native> CreateTimer( std::shared_ptr<PanelTarget> pTarget );

public:
    [[nodiscard]] HANDLE CreateNativeTimer( std::shared_ptr<Timer_Native> pTimer );
    void DestroyNativeTimer( HANDLE hTimer, bool waitForDestruction );
    void PostTimerEvent( std::shared_ptr<Timer_Native> pTimer );

private:
    TimerManager_Native();

private:
    HANDLE hTimerQueue_;
};

} // namespace smp
