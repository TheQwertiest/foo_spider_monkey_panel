#include <stdafx.h>

#include "event_manager.h"

#include <events/task_controller.h>
#include <panel/user_message.h>

#include <qwr/final_action.h>

namespace smp
{

EventManager& EventManager::Get()
{
    static EventManager em;
    return em;
}

void EventManager::AddWindow( HWND hWnd )
{
    std::unique_lock ul( taskControllerMapMutex_ );

    assert( !taskControllerMap_.contains( hWnd ) );
    taskControllerMap_.try_emplace( hWnd, nullptr );
}

void EventManager::RemoveWindow( HWND hWnd )
{
    std::unique_lock ul( taskControllerMapMutex_ );

    assert( taskControllerMap_.contains( hWnd ) );
    taskControllerMap_.erase( hWnd );
}

void EventManager::ClearEventQueue( HWND hWnd, std::shared_ptr<PanelTarget> pTarget )
{
    std::unique_lock ul( taskControllerMapMutex_ );

    assert( taskControllerMap_.contains( hWnd ) );
    taskControllerMap_.at( hWnd ) = std::make_shared<TaskController>( pTarget );
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

    if ( !pTaskController )
    {
        return false;
    }
    return pTaskController->ExecuteNextTask();
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

void EventManager::PutEvent( HWND hWnd, std::unique_ptr<EventBase> pEvent, EventPriority priority )
{
    std::scoped_lock sl( taskControllerMapMutex_ );

    auto taskControllerIt = taskControllerMap_.find( hWnd );
    if ( taskControllerIt == taskControllerMap_.end() || !taskControllerIt->second )
    {
        return;
    }

    auto pTaskController = taskControllerIt->second;
    pEvent->SetTarget( pTaskController->GetTarget() );
    pTaskController->AddRunnable( std::move( pEvent ), priority );

    PostMessage( hWnd, static_cast<UINT>( MiscMessage::run_next_event ), 0, 0 );
}

void EventManager::PutEventToAll( std::unique_ptr<EventBase> pEvent, EventPriority priority )
{
    std::scoped_lock sl( taskControllerMapMutex_ );

    for ( auto& [hLocalWnd, pTaskController]: taskControllerMap_ )
    {
        if ( !pTaskController )
        {
            continue;
        }

        auto pClonedEvent = pEvent->Clone();
        if ( !pClonedEvent )
        {
            assert( false );
            return;
        }

        pClonedEvent->SetTarget( pTaskController->GetTarget() );
        pTaskController->AddRunnable( std::move( pClonedEvent ), priority );

        PostMessage( hLocalWnd, static_cast<UINT>( MiscMessage::run_next_event ), 0, 0 );
    }
}

} // namespace smp
