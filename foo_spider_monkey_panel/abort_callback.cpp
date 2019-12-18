#include <stdafx.h>

#include "abort_callback.h"

namespace
{

class TimerManager
{
public:
    TimerManager()
        : hTimerQueue_( CreateTimerQueue() )
    {
        assert( hTimerQueue_ );
    }

    ~TimerManager()
    {
        (void)DeleteTimerQueueEx( hTimerQueue_, INVALID_HANDLE_VALUE );
    }

    HANDLE CreateTimer( WAITORTIMERCALLBACK callback, void* data, uint32_t timeoutSeconds )
    {
        HANDLE hTimer = nullptr;
        (void)CreateTimerQueueTimer(
            &hTimer,
            hTimerQueue_,
            callback,
            data,
            timeoutSeconds * 1000,
            0,
            WT_EXECUTEINTIMERTHREAD | WT_EXECUTEONLYONCE );
        return hTimer;
    }

    void DeleteTimer( HANDLE hTimer )
    {
        (void)DeleteTimerQueueTimer( hTimerQueue_, hTimer, INVALID_HANDLE_VALUE );
    }

private:
    HANDLE hTimerQueue_ = nullptr;
};

TimerManager g_timerManager;

} // namespace

namespace smp
{

GlobalAbortCallback& GlobalAbortCallback::GetInstance()
{
    static GlobalAbortCallback gac;
    return gac;
}

void GlobalAbortCallback::AddListener( pfc::event& listener )
{
    std::scoped_lock sl( listenerMutex_ );

    assert( !listeners_.count( &listener ) );
    listeners_.emplace( &listener, listener );
}

void GlobalAbortCallback::RemoveListener( pfc::event& listener )
{
    std::scoped_lock sl( listenerMutex_ );

    assert( listeners_.count( &listener ) );
    listeners_.erase( &listener );
}

void GlobalAbortCallback::Abort()
{
    {
        std::scoped_lock sl( listenerMutex_ );
        for ( auto& [key, listener]: listeners_ )
        {
            listener.get().set_state( true );
        }
    }
    abortImpl_.set();
}

bool GlobalAbortCallback::is_aborting() const
{
    return abortImpl_.is_aborting();
}

abort_callback_event GlobalAbortCallback::get_abort_event() const
{
    return abortImpl_.get_abort_event();
}

TimedAbortCallback::TimedAbortCallback( uint32_t timeoutSeconds )
    : hTimer_( g_timerManager.CreateTimer( timerProc, this, timeoutSeconds ) )
{
    GlobalAbortCallback::GetInstance().AddListener( abortEvent_ );
}

TimedAbortCallback::~TimedAbortCallback()
{
    if ( hTimer_ )
    {
        g_timerManager.DeleteTimer( hTimer_ );
    }
    GlobalAbortCallback::GetInstance().RemoveListener( abortEvent_ );
}

bool TimedAbortCallback::is_aborting() const
{
    return GlobalAbortCallback::GetInstance().is_aborting() || hasEnded_;
}

abort_callback_event TimedAbortCallback::get_abort_event() const
{
    return abortEvent_.get_handle();
}

VOID CALLBACK TimedAbortCallback::timerProc( PVOID lpParameter, BOOLEAN /*TimerOrWaitFired*/ )
{
    assert( lpParameter );
    auto& parent = *static_cast<TimedAbortCallback*>( lpParameter );

    parent.hasEnded_ = true;
    parent.abortEvent_.set_state( true );
}

} // namespace smp
