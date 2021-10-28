#include <stdafx.h>

#include "event_dispatcher.h"

#include <events/task_controller.h>
#include <panel/user_message.h>

#include <qwr/final_action.h>

namespace smp
{

EventDispatcher& EventDispatcher::Get()
{
    static EventDispatcher em;
    return em;
}

void EventDispatcher::AddWindow( HWND hWnd, std::shared_ptr<PanelTarget> pTarget )
{
    std::unique_lock ul( taskControllerMapMutex_ );

    assert( !taskControllerMap_.contains( hWnd ) );
    taskControllerMap_.try_emplace( hWnd, std::make_shared<TaskController>( pTarget ) );
    nextEventMsgStatusMap_.try_emplace( hWnd, true );
}

void EventDispatcher::RemoveWindow( HWND hWnd )
{
    std::unique_lock ul( taskControllerMapMutex_ );

    assert( taskControllerMap_.contains( hWnd ) );
    taskControllerMap_.erase( hWnd );
    nextEventMsgStatusMap_.erase( hWnd );
}

void EventDispatcher::NotifyAllAboutExit()
{
    std::vector<HWND> hWnds;
    hWnds.reserve( taskControllerMap_.size() );

    {
        std::scoped_lock sl( taskControllerMapMutex_ );
        for ( auto& [hLocalWnd, pTaskController]: taskControllerMap_ )
        {
            hWnds.emplace_back( hLocalWnd );
        }
    }

    for ( const auto& hWnd: hWnds )
    {
        SendMessage( hWnd, static_cast<UINT>( smp::InternalSyncMessage::prepare_for_exit ), 0, 0 );
    }
}

bool EventDispatcher::IsRequestEventMessage( UINT msg )
{
    return ( msg == static_cast<UINT>( InternalSyncMessage::run_next_event ) );
}

bool EventDispatcher::ProcessNextEvent( HWND hWnd )
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

void EventDispatcher::RequestNextEvent( HWND hWnd )
{
    std::scoped_lock sl( taskControllerMapMutex_ );

    auto taskControllerIt = taskControllerMap_.find( hWnd );
    if ( taskControllerIt == taskControllerMap_.end() || !taskControllerIt->second )
    { // task controller might be missing when invoked before window initialization
        return;
    }

    RequestNextEventImpl( hWnd, *taskControllerIt->second, sl );
}

void EventDispatcher::RequestNextEventImpl( HWND hWnd, TaskController& taskController, std::scoped_lock<std::mutex>& proof )
{
    if ( !taskController.HasTasks() )
    {
        return;
    }

    const auto isWaitingForMsgIt = nextEventMsgStatusMap_.find( hWnd );
    if ( isWaitingForMsgIt == nextEventMsgStatusMap_.end() )
    {
        assert( false );
        return;
    }

    if ( isWaitingForMsgIt->second )
    {
        isWaitingForMsgIt->second = false;
        PostMessage( hWnd, static_cast<UINT>( InternalSyncMessage::run_next_event ), 0, 0 );
    }
    else
    {
        isWaitingForMsgIt->second = false;
    }
}

void EventDispatcher::OnRequestEventMessageReceived( HWND hWnd )
{
    std::scoped_lock sl( taskControllerMapMutex_ );

    auto isWaitingForMsgIt = nextEventMsgStatusMap_.find( hWnd );
    if ( isWaitingForMsgIt == nextEventMsgStatusMap_.end() )
    {
        return;
    }

    isWaitingForMsgIt->second = true;
}

void EventDispatcher::PutRunnable( HWND hWnd, std::shared_ptr<Runnable> pRunnable, EventPriority priority )
{
    std::scoped_lock sl( taskControllerMapMutex_ );

    auto taskControllerIt = taskControllerMap_.find( hWnd );
    if ( taskControllerIt == taskControllerMap_.end() || !taskControllerIt->second )
    {
        return;
    }

    auto pTaskController = taskControllerIt->second;
    pTaskController->AddRunnable( std::move( pRunnable ), priority );

    RequestNextEventImpl( hWnd, *pTaskController, sl );
}

void EventDispatcher::PutEvent( HWND hWnd, std::unique_ptr<EventBase> pEvent, EventPriority priority )
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

    RequestNextEventImpl( hWnd, *pTaskController, sl );
}

void EventDispatcher::PutEventToAll( std::unique_ptr<EventBase> pEvent, EventPriority priority )
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

        RequestNextEventImpl( hLocalWnd, *pTaskController, sl );
    }
}

void EventDispatcher::PutEventToOthers( HWND hWnd, std::unique_ptr<EventBase> pEvent, EventPriority priority )
{
    std::scoped_lock sl( taskControllerMapMutex_ );

    for ( auto& [hLocalWnd, pTaskController]: taskControllerMap_ )
    {
        if ( !pTaskController || hLocalWnd == hWnd )
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

        RequestNextEventImpl( hLocalWnd, *pTaskController, sl );
    }
}

void EventDispatcher::NotifyOthers( HWND hWnd, std::unique_ptr<EventBase> pEvent )
{
    std::vector<std::pair<HWND, std::unique_ptr<EventBase>>> hWndToEvent;
    hWndToEvent.reserve( taskControllerMap_.size() );

    {
        std::scoped_lock sl( taskControllerMapMutex_ );
        for ( auto& [hLocalWnd, pTaskController]: taskControllerMap_ )
        {
            if ( !pTaskController || hLocalWnd == hWnd )
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

            hWndToEvent.emplace_back( hLocalWnd, std::move( pClonedEvent ) );
        }
    }

    for ( const auto& [hWnd, pClonedEvent]: hWndToEvent )
    {
        SendMessage( hWnd, static_cast<UINT>( smp::InternalSyncMessage::legacy_notify_others ), 0, reinterpret_cast<LPARAM>( pClonedEvent.get() ) );
    }
}

} // namespace smp
