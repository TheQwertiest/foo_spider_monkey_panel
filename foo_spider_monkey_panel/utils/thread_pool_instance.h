#pragma once

#include <qwr/thread_pool.h>

namespace smp
{

[[nodiscard]] qwr::ThreadPool& GetThreadPoolInstance();

} // namespace smp
