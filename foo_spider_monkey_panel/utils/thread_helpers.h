#pragma once

#include <thread>

namespace smp::utils
{

void SetThreadName( std::thread& thread, const char* threadName );

}
