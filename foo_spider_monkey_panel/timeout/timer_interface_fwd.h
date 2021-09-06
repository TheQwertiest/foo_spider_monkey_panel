#pragma once

#include <timeout/time_types.h>

#if 0

namespace smp
{

struct TimerNotifyTask_Native;
class Timer_Native;
class TimerManager_Native;

using TimerImpl = Timer_Native;
using TimerManagerImpl = TimerManager_Native;

}

#else

namespace smp
{

struct TimerNotifyTask;
class Timer;
class TimerManager;

using TimerNotifyTaskImpl = TimerNotifyTask;
using TimerImpl = Timer;
using TimerManagerImpl = TimerManager;

} // namespace smp

#endif