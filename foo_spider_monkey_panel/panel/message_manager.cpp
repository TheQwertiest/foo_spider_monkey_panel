#include <stdafx.h>

#include "message_manager.h"

namespace smp::panel
{

MessageManager& MessageManager::Get()
{
    static MessageManager sm_instance;
    return sm_instance;
}

void MessageManager::AddWindow( HWND hWnd )
{
    std::scoped_lock sl( wndDataMutex_ );

    assert( !wndDataMap_.contains( hWnd ) );
    wndDataMap_.emplace( hWnd, WindowData{} );
}

void MessageManager::RemoveWindow( HWND hWnd )
{
    std::scoped_lock sl( wndDataMutex_ );

    assert( wndDataMap_.contains( hWnd ) );
    wndDataMap_.erase( hWnd );
}

void MessageManager::EnableAsyncMessages( HWND hWnd )
{
    DisableAsyncMessages( hWnd );

    std::scoped_lock sl( wndDataMutex_ );

    assert( wndDataMap_.contains( hWnd ) );
    auto& windowData = wndDataMap_[hWnd];

    ++( windowData.currentGeneration );
    windowData.isAsyncEnabled = true;
}

void MessageManager::DisableAsyncMessages( HWND hWnd )
{
    std::scoped_lock sl( wndDataMutex_ );

    assert( wndDataMap_.contains( hWnd ) );
    auto& windowData = wndDataMap_[hWnd];

    windowData.asyncMsgQueue.clear();
    windowData.isAsyncEnabled = false;
}

bool MessageManager::IsAsyncMessage( UINT msg )
{
    return ( msg == static_cast<UINT>( MiscMessage::run_task_async ) );
}

std::optional<MessageManager::AsyncMessage> MessageManager::ClaimAsyncMessage( HWND hWnd, UINT msg, WPARAM wp, LPARAM )
{
    assert( IsAsyncMessage( msg ) );
    std::scoped_lock sl( wndDataMutex_ );

    assert( wndDataMap_.contains( hWnd ) );
    auto& [currentGeneration, asyncMsgQueue, isAsyncEnabled] = wndDataMap_[hWnd];

    if ( currentGeneration != static_cast<uint32_t>( wp ) || asyncMsgQueue.empty() )
    {
        return std::nullopt;
    }

    auto curMsg = asyncMsgQueue.front();
    asyncMsgQueue.pop_front();
    return curMsg;
}

void MessageManager::RequestNextAsyncMessage( HWND hWnd )
{
    std::scoped_lock sl( wndDataMutex_ );

    if ( !wndDataMap_.contains( hWnd ) )
    { // this is possible when invoked before window initialization
        return;
    }

    const auto& [currentGeneration, asyncMsgQueue, isAsyncEnabled] = wndDataMap_[hWnd];

    if ( !asyncMsgQueue.empty() )
    {
        PostMessage( hWnd, static_cast<UINT>( MiscMessage::run_task_async ), currentGeneration, 0 );
    }
}

void MessageManager::PostMsg( HWND hWnd, UINT msg, WPARAM wp, LPARAM lp )
{
    assert( IsAllowedAsyncMessage( msg ) );
    std::scoped_lock sl( wndDataMutex_ );

    assert( wndDataMap_.contains( hWnd ) );
    PostMsgImpl( hWnd, wndDataMap_[hWnd], msg, wp, lp );
}

void MessageManager::PostMsgToAll( UINT msg, WPARAM wp, LPARAM lp )
{
    assert( IsAllowedAsyncMessage( msg ) );
    std::scoped_lock sl( wndDataMutex_ );

    for ( auto& [hWnd, wndData]: wndDataMap_ )
    {
        PostMsgImpl( hWnd, wndData, msg, wp, lp );
    }
}

void MessageManager::SendMsgToAll( UINT msg, WPARAM wp, LPARAM lp )
{
    std::vector<HWND> hWnds;
    hWnds.reserve( wndDataMap_.size() );

    {
        std::scoped_lock sl( wndDataMutex_ );
        for ( const auto& [hWnd, wndData]: wndDataMap_ )
        {
            hWnds.emplace_back( hWnd );
        }
    }

    for ( const auto& hWnd: hWnds )
    {
        SendMessage( hWnd, msg, wp, lp );
    }
}

void MessageManager::SendMsgToOthers( HWND hWnd_except, UINT msg, WPARAM wp, LPARAM lp )
{
    std::vector<HWND> hWnds;
    hWnds.reserve( wndDataMap_.size() );

    {
        std::scoped_lock sl( wndDataMutex_ );
        for ( const auto& [hWnd, wndData]: wndDataMap_ )
        {
            hWnds.emplace_back( hWnd );
        }
    }

    for ( const auto& hWnd: hWnds )
    {
        if ( hWnd != hWnd_except )
        {
            SendMessage( hWnd, msg, wp, lp );
        }
    }
}

bool MessageManager::IsAllowedAsyncMessage( UINT msg )
{
    return ( IsInEnumRange<InternalAsyncMessage>( msg ) );
}

void MessageManager::PostMsgImpl( HWND hWnd, WindowData& windowData, UINT msg, WPARAM wp, LPARAM lp )
{
    auto& [currentGeneration, asyncMsgQueue, isAsyncEnabled] = windowData;
    if ( !isAsyncEnabled )
    {
        return;
    }

    asyncMsgQueue.emplace_back( msg, wp, lp );
    if ( !PostMessage( hWnd, static_cast<INT>( MiscMessage::run_task_async ), currentGeneration, 0 ) )
    {
        asyncMsgQueue.pop_back();
    }
}

} // namespace smp::panel
