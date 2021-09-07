#pragma once

#include <chrono>

namespace smp
{

using TimeStamp = std::chrono::time_point<std::chrono::steady_clock>;
using TimeDuration = TimeStamp::duration;

} // namespace smp
