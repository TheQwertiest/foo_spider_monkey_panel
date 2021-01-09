#pragma once

#include <qwr/thread_pool.h>

namespace smp
{

qwr::ThreadPool& GetThreadPoolInstance();

} // namespace smp
