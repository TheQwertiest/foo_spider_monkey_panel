#include <stdafx.h>

#include "task_controller.h"

#include <panel/user_message.h>

#include <qwr/final_action.h>

namespace smp::panel
{

std::atomic<uint64_t> Task::g_currentTaskNumber = 0;

Task::Task( EventPriority priority )
    : taskNumber_( g_currentTaskNumber++ )
    , priority_( priority )
{
}

EventPriority Task::GetPriority() const
{
    return priority_;
}

int64_t Task::GetTaskNumber() const
{
    return taskNumber_;
}

bool Task::PriorityCompare::operator()( const std::shared_ptr<Task>& a, const std::shared_ptr<Task>& b ) const
{
    const auto prioA = a->priority_;
    const auto prioB = b->priority_;
    return ( ( prioA > prioB ) || ( prioA == prioB && ( a->taskNumber_ < b->taskNumber_ ) ) );
}

RunnableTask::RunnableTask( std::shared_ptr<Runnable> pRunnable, EventPriority priority )
    : Task( priority )
    , pRunnable_( pRunnable )
{
}

void RunnableTask::Run( js_panel_window& panelWindow )
{
    assert( pRunnable_ );
    pRunnable_->Run( panelWindow );
}

void TaskController::AddTask( std::shared_ptr<Task> pTask )
{
    std::scoped_lock sl( tasksMutex_ );

    assert( pTask );
    const auto [it, wasInserted] = tasks_.emplace( pTask );
    pTask->taskIterator_ = it;
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

bool TaskController::ExecuteNextTask( js_panel_window& panelWindow )
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
    { // allow other tasks to be queued
        ul.unlock();
        qwr::final_action autoLock( [&] {
            ul.lock();
        } );

        pTask->Run( panelWindow );
    }

    return true;
}

} // namespace smp::panel
