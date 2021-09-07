#include <stdafx.h>

#include "timer_native.h"

#include <timeout/timer_manager_native.h>

using namespace smp;

namespace smp
{

Timer_Native::Timer_Native( TimerManager_Native& pParent, std::shared_ptr<PanelTarget> pTarget )
    : pParent_( pParent )
    , pTarget_( pTarget )
{
}

void Timer_Native::Start( TimerNotifyTask& task, const TimeStamp& when )
{
    assert( core_api::is_main_thread() );

    if ( hTimer_ )
    {
        pParent_.DestroyNativeTimer( hTimer_, false );
        hTimer_ = nullptr;
    }

    pTask_ = &task;
    ++generation_;

    executeAt_ = when;

    hTimer_ = pParent_.CreateNativeTimer( shared_from_this() );
}

void Timer_Native::Fire( uint64_t generation )
{
    // Save self, since it can be destroyed in `Notify` callback
    auto selfSaver = shared_from_this();

    if ( generation != generation_ )
    {
        return;
    }

    if ( pTask_ )
    {
        pTask_->Notify();
    }

    if ( generation == generation_ )
    {
        pTask_ = nullptr;
    }
}

void Timer_Native::Cancel( bool waitForDestruction )
{
    if ( hTimer_ )
    {
        pParent_.DestroyNativeTimer( hTimer_, waitForDestruction );
        hTimer_ = nullptr;
    }

    pTask_ = nullptr;
    ++generation_;
}

VOID CALLBACK Timer_Native::TimerProc( PVOID lpParameter, BOOLEAN /*TimerOrWaitFired*/ )
{
    auto pTimer = reinterpret_cast<Timer_Native*>( lpParameter );
    assert( pTimer );

    pTimer->pParent_.PostTimerEvent( pTimer->shared_from_this() );
}

PanelTarget& Timer_Native::Target() const
{
    assert( pTarget_ );
    return *pTarget_;
}

const TimeStamp& Timer_Native::When() const
{
    return executeAt_;
}

uint64_t Timer_Native::Generation() const
{
    return generation_;
}

} // namespace smp
