#include <stdafx.h>

#include "thread_pool_instance.h"

namespace smp
{

qwr::ThreadPool& GetThreadPoolInstance()
{
    static qwr::ThreadPool tp;
    return tp;
}

} // namespace smp