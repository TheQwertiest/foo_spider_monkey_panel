#include <stdafx.h>

#include "timeout.h"

namespace smp
{

Timeout::Timeout( uint32_t id, TimeDuration interval, bool isRepeated, std::shared_ptr<mozjs::JsAsyncTask> task )
    : id_( id )
    , interval_( interval )
    , task_( task )
    , isRepeated_( isRepeated )
{
}

void Timeout::SetWhen( const TimeStamp& baseTime, const TimeDuration& delay )
{
    executeAt_ = baseTime + delay;
}

void Timeout::SetFiringId( uint32_t firingId )
{
    firingId_ = firingId;
}

void Timeout::DisableRepeat()
{
    isRepeated_ = false;
}

void Timeout::MarkAsStopped()
{
    isStopped_ = true;
}

void Timeout::SetRunningState( bool isRunning )
{
    isRunning_ = isRunning;
}

uint32_t Timeout::Id() const
{
    return id_;
}

std::shared_ptr<mozjs::JsAsyncTask> Timeout::Task() const
{
    return task_;
}

const smp::TimeStamp& Timeout::When() const
{
    return executeAt_;
}

bool Timeout::IsRunning() const
{
    return isRunning_;
}

bool Timeout::IsRepeated() const
{
    return isRepeated_;
}

TimeDuration Timeout::Interval() const
{
    return interval_;
}

uint32_t Timeout::GetFiringId()
{
    return firingId_;
}

} // namespace smp
