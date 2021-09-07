#include <stdafx.h>

#include "timer_custom.h"

#include <panel/js_panel_window.h>
#include <timeout/timer_manager_custom.h>

#include <qwr/thread_helpers.h>

using namespace smp;

namespace smp
{

TimerHolder::TimerHolder( std::shared_ptr<Timer_Custom> pTimer )
    : pTimer_( pTimer )
    , executeAt_( pTimer_->When() )
{
}

TimerHolder::~TimerHolder()
{
    ResetValue();
}

void TimerHolder::ResetValue()
{
    if ( pTimer_ )
    {
        pTimer_->SetHolder( nullptr );
        pTimer_.reset();
    }
}

std::shared_ptr<Timer_Custom> TimerHolder::Value() const
{
    return pTimer_;
}

const TimeStamp& TimerHolder::When() const
{
    return executeAt_;
}

Timer_Custom::Timer_Custom( TimerManager_Custom& pParent, std::shared_ptr<PanelTarget> pTarget )
    : pParent_( pParent )
    , pTarget_( pTarget )
{
}

void Timer_Custom::Start( TimerNotifyTask& task, const TimeStamp& when )
{
    assert( core_api::is_main_thread() );

    std::scoped_lock sl( mutex_ );

    auto self = shared_from_this();

    pParent_.RemoveTimer( self );

    pTask_ = &task;
    ++generation_;

    executeAt_ = when;

    pParent_.AddTimer( self );
}

void Timer_Custom::Fire( uint64_t generation )
{
    // Save self, since it can be destroyed in `Notify` callback
    auto selfSaver = shared_from_this();
    decltype( pTask_ ) pTask = nullptr;

    {
        // Don't fire callbacks when the mutex is locked.
        // If some other thread Cancels after this, they're just too late.
        std::scoped_lock sl( mutex_ );
        if ( generation != generation_ )
        {
            return;
        }

        pTask = pTask_;
    }

    if ( pTask )
    {
        pTask->Notify();
    }

    {
        std::scoped_lock sl( mutex_ );
        if ( generation == generation_ )
        {
            pTask_ = nullptr;
        }
    }
}

void Timer_Custom::Cancel( bool /*waitForDestruction*/ )
{
    std::scoped_lock sl( mutex_ );

    auto self = shared_from_this();

    pParent_.RemoveTimer( self );

    pTask_ = nullptr;
    ++generation_;
}

void Timer_Custom::SetHolder( TimerHolder* pHolder )
{
    pHolder_ = pHolder;
}

PanelTarget& Timer_Custom::Target() const
{
    assert( pTarget_ );
    return *pTarget_;
}

const TimeStamp& Timer_Custom::When() const
{
    return executeAt_;
}

uint64_t Timer_Custom::Generation() const
{
    return generation_;
}

TimerHolder* Timer_Custom::Holder() const
{
    return pHolder_;
}

} // namespace smp
