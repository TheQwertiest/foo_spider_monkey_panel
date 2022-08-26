// based on https://searchfox.org/mozilla-central/source/xpcom/threads/TaskController.cpp

#include <stdafx.h>

#include "task_controller.h"

#include <panel/user_message.h>

#include <qwr/final_action.h>

namespace smp
{

std::atomic<uint64_t> Task::g_currentTaskNumber = 0;

Task::Task( EventPriority priority )
    : taskNumber_( g_currentTaskNumber++ )
    , priority_( priority )
{
}

bool Task::PriorityCompare::operator()( const std::shared_ptr<Task>& a, const std::shared_ptr<Task>& b ) const
{
    const auto prioA = a->priority_;
    const auto prioB = b->priority_;
    return ( ( prioA > prioB ) || ( prioA == prioB && ( a->taskNumber_ < b->taskNumber_ ) ) );
}

RunnableTask::RunnableTask( std::shared_ptr<Runnable> pRunnable, EventPriority priority )
    : Task( priority )
    , pRunnable_( std::move( pRunnable ) )
{
}

void RunnableTask::Run()
{
    assert( pRunnable_ );
    pRunnable_->Run();
}

TaskController::TaskController( std::shared_ptr<PanelTarget> pTarget )
    : pTarget_( std::move( pTarget ) )
{
}

std::shared_ptr<PanelTarget> TaskController::GetTarget()
{
    return pTarget_;
}

void TaskController::AddTask( std::shared_ptr<Task> pTask )
{
    std::scoped_lock sl( tasksMutex_ );

    assert( pTask );
    const auto [it, wasInserted] = tasks_.emplace( pTask );
    pTask->taskIterator_ = it;

    // HACK: dirty way to deduplicate unique events
    if ( it == tasks_.begin() )
    {
        return;
    }

    const auto priority = pTask->priority_;
    if ( priority == EventPriority::kResize )
    {
        auto prevIt = std::prev( it );
        auto pPrevTask = *prevIt;
        if ( !pPrevTask->isInProgress_ && pPrevTask->priority_ == priority )
        {
            tasks_.erase( prevIt );
        }
    }
}

void TaskController::AddRunnable( std::shared_ptr<Runnable> pRunnable, EventPriority priority )
{
    assert( pRunnable );
    AddTask( std::make_shared<RunnableTask>( pRunnable, priority ) );
}

bool TaskController::HasTasks() const
{
    std::unique_lock ul( tasksMutex_ );
    return !tasks_.empty();
}

bool TaskController::ExecuteNextTask()
{
    assert( core_api::is_main_thread() );

    std::unique_lock ul( tasksMutex_ );

    if ( tasks_.empty() )
    {
        return false;
    }

    // in case we destroy ourself during task run
    auto selfSaver = shared_from_this();

    auto it = ranges::find_if( tasks_, []( const auto pTask ) { return !pTask->isInProgress_; } );
    if ( it == tasks_.end() )
    {
        return false;
    }

    auto pTask = *it;
    tasks_.erase( pTask->taskIterator_ );

    pTask->taskIterator_ = tasks_.end();
    pTask->isInProgress_ = true;

    { // allow other tasks to be queued
        ul.unlock();
        qwr::final_action autoLock( [&] {
            ul.lock();
        } );

        pTask->Run();
    }

    pTask->isInProgress_ = false;

    return true;
}

} // namespace smp
