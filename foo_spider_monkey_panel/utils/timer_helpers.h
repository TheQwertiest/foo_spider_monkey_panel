#pragma once

#include <string>

namespace smp::timer
{

/// @brief Execute function with incrementing timeout until it succeeds or max timeout is reached.
/// @param[in] callbackFn Callback function, must return bool.
template <typename T>
void CallWithIncrementingTimeout( T&& callbackFn, size_t baseTimeoutMs, size_t maxTimeoutMs, float timeoutIncrementFactor = 1.5 )
{
    if ( !baseTimeoutMs && callbackFn() )
    {
        return;
    }

    const size_t endTime = static_cast<size_t>( GetTickCount() ) + maxTimeoutMs;

    bool isSuccess = false;
    size_t curIntervalInMs = (baseTimeoutMs ? baseTimeoutMs : 50);
    size_t curTime = static_cast<size_t>( GetTickCount() );

    while ( curTime < endTime )
    {
        Sleep( curIntervalInMs );
        curIntervalInMs = static_cast<size_t>( timeoutIncrementFactor * curIntervalInMs );
        if ( curIntervalInMs > ( endTime - curTime ) )
        {
            curIntervalInMs = endTime - curTime;
        }

        if ( callbackFn() )
        {
            return;
        }

        curTime = static_cast<size_t>( GetTickCount() );
    }
}

} // namespace smp::timer
