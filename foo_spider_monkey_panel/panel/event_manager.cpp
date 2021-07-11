#include <stdafx.h>

#include "event_manager.h"

#include <panel/task_controller.h>
#include <panel/user_message.h>

#include <qwr/final_action.h>

namespace smp::panel
{

EventManager& EventManager::Get()
{
    static EventManager em;
    return em;
}

void EventManager::AddWindow( HWND hWnd, js_panel_window& panelWindow )
{
    {
        std::unique_lock ul( taskControllerMapMutex_ );

        assert( !taskControllerMap_.contains( hWnd ) );
        taskControllerMap_.try_emplace( hWnd, std::make_shared<TaskController>() );
    }

    assert( !windowMap_.contains( hWnd ) );
    windowMap_.try_emplace( hWnd, &panelWindow );
}

void EventManager::RemoveWindow( HWND hWnd )
{
    {
        std::unique_lock ul( taskControllerMapMutex_ );

        assert( taskControllerMap_.contains( hWnd ) );
        taskControllerMap_.erase( hWnd );
    }

    assert( windowMap_.contains( hWnd ) );
    windowMap_.erase( hWnd );
}

void EventManager::EnableEventQueue( HWND hWnd )
{
    std::unique_lock ul( taskControllerMapMutex_ );

    assert( taskControllerMap_.contains( hWnd ) );
    taskControllerMap_.at( hWnd ) = std::make_shared<TaskController>();
}

void EventManager::DisableEventQueue( HWND hWnd )
{
    std::unique_lock ul( taskControllerMapMutex_ );

    assert( taskControllerMap_.contains( hWnd ) );
    taskControllerMap_.at( hWnd ).reset();
}

bool EventManager::IsRequestEventMessage( UINT msg )
{
    return ( msg == static_cast<UINT>( MiscMessage::run_next_event ) );
}

bool EventManager::ProcessNextEvent( HWND hWnd )
{
    auto pTaskController = [&] {
        std::unique_lock ul( taskControllerMapMutex_ );

        assert( taskControllerMap_.contains( hWnd ) );
        return taskControllerMap_.at( hWnd );
    }();

    assert( windowMap_.contains( hWnd ) );
    return pTaskController->ExecuteNextTask( *windowMap_.at( hWnd ) );
}

void EventManager::RequestNextEvent( HWND hWnd )
{
    std::scoped_lock sl( taskControllerMapMutex_ );

    auto taskControllerIt = taskControllerMap_.find( hWnd );
    if ( taskControllerIt == taskControllerMap_.end() || !taskControllerIt->second )
    { // task controller might be missing when invoked before window initialization
        return;
    }

    if ( !taskControllerIt->second->HasTasks() )
    {
        return;
    }

    PostMessage( hWnd, static_cast<UINT>( MiscMessage::run_next_event ), 0, 0 );
}

void EventManager::PutEvent( HWND hWnd, std::unique_ptr<Runnable> event, EventPriority priority )
{
    std::scoped_lock sl( taskControllerMapMutex_ );

    auto taskControllerIt = taskControllerMap_.find( hWnd );
    if ( taskControllerIt == taskControllerMap_.end() || !taskControllerIt->second )
    {
        return;
    }

    taskControllerIt->second->AddRunnable( std::move( event ), priority );
}

void EventManager::PutEventToOthers( HWND hWnd, std::unique_ptr<Runnable> event, EventPriority priority )
{
    std::scoped_lock sl( taskControllerMapMutex_ );

    auto pTask = std::make_shared<RunnableTask>( std::move( event ), priority );

    for ( auto& [hLocalWnd, pTaskController]: taskControllerMap_ )
    {
        if ( hLocalWnd == hWnd || !pTaskController )
        {
            continue;
        }
        pTaskController->AddTask( pTask );
    }
}

void EventManager::PutEventToAll( std::unique_ptr<Runnable> event, EventPriority priority )
{
    std::scoped_lock sl( taskControllerMapMutex_ );

    auto pTask = std::make_shared<RunnableTask>( std::move( event ), priority );

    for ( auto& [hLocalWnd, pTaskController]: taskControllerMap_ )
    {
        if ( !pTaskController )
        {
            continue;
        }
        pTaskController->AddTask( pTask );
    }
}

} // namespace smp::panel
