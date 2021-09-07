#include <stdafx.h>

#include "timer_manager_native.h"

#include <events/event_dispatcher.h>
#include <events/event_timer.h>
#include <panel/js_panel_window.h>
#include <timeout/timer_native.h>
#include <utils/logging.h>

#include <qwr/winapi_error_helpers.h>

using namespace smp;

namespace smp
{

TimerManager_Native::TimerManager_Native()
    : hTimerQueue_( CreateTimerQueue() )
{
}

void TimerManager_Native::Finalize()
{
    DeleteTimerQueueEx( hTimerQueue_, INVALID_HANDLE_VALUE );
}

TimerManager_Native& TimerManager_Native::Get()
{
    static TimerManager_Native tm;
    return tm;
}

const TimeDuration& TimerManager_Native::GetAllowedEarlyFiringTime()
{
    static constexpr TimeDuration earlyDelay{ std::chrono::microseconds( 10 ) };
    return earlyDelay;
}

std::unique_ptr<Timer_Native> TimerManager_Native::CreateTimer( std::shared_ptr<PanelTarget> pTarget )
{
    return std::unique_ptr<Timer_Native>( new Timer_Native( *this, pTarget ) );
}

HANDLE TimerManager_Native::CreateNativeTimer( std::shared_ptr<Timer_Native> pTimer )
{
    assert( pTimer );

    const auto timeDiff = pTimer->When() - TimeStamp::clock::now();
    if ( timeDiff < GetAllowedEarlyFiringTime() )
    {
        PostTimerEvent( pTimer );
        return nullptr;
    }

    HANDLE hTimer;
    BOOL bRet = CreateTimerQueueTimer(
        &hTimer,
        hTimerQueue_,
        Timer_Native::TimerProc,
        pTimer.get(),
        static_cast<DWORD>( std::chrono::duration_cast<std::chrono::milliseconds>( timeDiff ).count() ),
        0,
        WT_EXECUTEINTIMERTHREAD | WT_EXECUTEONLYONCE );

    try
    {
        qwr::error::CheckWinApi( bRet, "CreateTimerQueueTimer" );
    }
    catch ( const qwr::QwrException& e )
    {
        utils::LogError( e.what() );
        return nullptr;
    }

    return hTimer;
}

void TimerManager_Native::DestroyNativeTimer( HANDLE hTimer, bool waitForDestruction )
{
    assert( hTimerQueue_ );
    assert( hTimer );
    (void)DeleteTimerQueueTimer( hTimerQueue_, hTimer, waitForDestruction ? INVALID_HANDLE_VALUE : nullptr );
}

void TimerManager_Native::PostTimerEvent( std::shared_ptr<Timer_Native> pTimer )
{
    EventDispatcher::Get().PutEvent( pTimer->Target().GetHwnd(), std::make_unique<Event_Timer>( pTimer, pTimer->Generation() ) );
}

} // namespace smp
