#include <stdafx.h>
#include "message_manager.h"

namespace smp::panel
{

message_manager& message_manager::instance()
{
    static message_manager sm_instance;
    return sm_instance;
}

void message_manager::AddWindow( HWND hWnd )
{
    std::scoped_lock sl( wndDataMutex_ );

    assert( !wndDataMap_.count( hWnd ) );
    wndDataMap_.emplace( hWnd, WindowData{} );
}

void message_manager::RemoveWindow( HWND hWnd )
{
    std::scoped_lock sl( wndDataMutex_ );

    assert( wndDataMap_.count( hWnd ) );
    wndDataMap_.erase( hWnd );
}

void message_manager::EnableAsyncMessages( HWND hWnd )
{
    DisableAsyncMessages( hWnd );

    std::scoped_lock sl( wndDataMutex_ );

    assert( wndDataMap_.count( hWnd ) );
    auto& windowData = wndDataMap_[hWnd];

    ++( windowData.currentGeneration );
    windowData.isAsyncEnabled = true;
}

void message_manager::DisableAsyncMessages( HWND hWnd )
{
    std::scoped_lock sl( wndDataMutex_ );

    assert( wndDataMap_.count( hWnd ) );
    auto& windowData = wndDataMap_[hWnd];

    windowData.callbackMsgQueue.clear();
    windowData.asyncMsgQueue.clear();
    windowData.isAsyncEnabled = false;
}

bool message_manager::IsAsyncMessage( UINT msg )
{
    return ( msg == static_cast<UINT>( MiscMessage::run_task_async ) );
}

std::optional<message_manager::AsyncMessage> message_manager::ClaimAsyncMessage( HWND hWnd, UINT msg, WPARAM wp, LPARAM )
{
    assert( IsAsyncMessage( msg ) );
    std::scoped_lock sl( wndDataMutex_ );

    assert( wndDataMap_.count( hWnd ) );
    auto& [currentGeneration, callbackMsgQueue, asyncMsgQueue, isAsyncEnabled] = wndDataMap_[hWnd];

    if ( currentGeneration != static_cast<uint32_t>( wp ) || asyncMsgQueue.empty() )
    {
        return std::nullopt;
    }

    auto curMsg = asyncMsgQueue.front();
    asyncMsgQueue.pop_front();
    return curMsg;
}

std::shared_ptr<CallbackData> message_manager::ClaimCallbackMessageData( HWND hWnd, CallbackMessage msg )
{
    std::scoped_lock sl( wndDataMutex_ );

    assert( wndDataMap_.count( hWnd ) );
    auto& [currentGeneration, callbackMsgQueue, asyncMsgQueue, isAsyncEnabled] = wndDataMap_[hWnd];

    const auto it = ranges::find_if( callbackMsgQueue, [msgId = msg]( const auto& msg ) {
        return msg.id == msgId;
    } );
    assert( it != callbackMsgQueue.cend() );

    auto msgData = it->pData;
    callbackMsgQueue.erase( it );

    return msgData;
}

void message_manager::RequestNextAsyncMessage( HWND hWnd )
{
    std::scoped_lock sl( wndDataMutex_ );

    if ( !wndDataMap_.count( hWnd ) )
    { // this is possible when invoked before window initialization
        return;
    }

    const auto& [currentGeneration, callbackMsgQueue, asyncMsgQueue, isAsyncEnabled] = wndDataMap_[hWnd];

    if ( !asyncMsgQueue.empty() )
    {
        PostMessage( hWnd, static_cast<UINT>( MiscMessage::run_task_async ), currentGeneration, 0 );
    }
}

void message_manager::post_msg( HWND hWnd, UINT msg, WPARAM wp, LPARAM lp )
{
    assert( IsAllowedAsyncMessage( msg ) );
    std::scoped_lock sl( wndDataMutex_ );

    assert( wndDataMap_.count( hWnd ) );
    post_msg_impl( hWnd, wndDataMap_[hWnd], msg, wp, lp );
}

void message_manager::post_msg_to_all( UINT msg, WPARAM wp, LPARAM lp )
{
    assert( IsAllowedAsyncMessage( msg ) );
    std::scoped_lock sl( wndDataMutex_ );

    for ( auto& [hWnd, wndData] : wndDataMap_ )
    {
        post_msg_impl( hWnd, wndData, msg, wp, lp );
    }
}

void message_manager::post_callback_msg( HWND hWnd, CallbackMessage msg, std::unique_ptr<CallbackData> data )
{
    std::shared_ptr<CallbackData> sharedData( data.release() );

    std::scoped_lock sl( wndDataMutex_ );
    assert( wndDataMap_.count( hWnd ) );

    post_callback_msg_impl( hWnd, wndDataMap_[hWnd], msg, sharedData );
}

void message_manager::post_callback_msg_to_all( CallbackMessage msg, std::unique_ptr<CallbackData> data )
{
    if ( wndDataMap_.empty() )
    {
        return;
    }

    std::shared_ptr<CallbackData> sharedData( data.release() );

    std::scoped_lock sl( wndDataMutex_ );
    for ( auto& [hWnd, wndData] : wndDataMap_ )
    {
        post_callback_msg_impl( hWnd, wndData, msg, sharedData );
    }
}

void message_manager::send_msg_to_all( UINT msg, WPARAM wp, LPARAM lp )
{
    std::vector<HWND> hWnds;
    hWnds.reserve( wndDataMap_.size() );

    {
        std::scoped_lock sl( wndDataMutex_ );
        for ( const auto& [hWnd, wndData] : wndDataMap_ )
        {
            hWnds.emplace_back( hWnd );
        }
    }

    for ( const auto& hWnd : hWnds )
    {
        SendMessage( hWnd, msg, wp, lp );
    }
}

void message_manager::send_msg_to_others( HWND hWnd_except, UINT msg, WPARAM wp, LPARAM lp )
{
    std::vector<HWND> hWnds;
    hWnds.reserve( wndDataMap_.size() );

    {
        std::scoped_lock sl( wndDataMutex_ );
        for ( const auto& [hWnd, wndData] : wndDataMap_ )
        {
            hWnds.emplace_back( hWnd );
        }
    }

    for ( const auto& hWnd : hWnds )
    {
        if ( hWnd != hWnd_except )
        {
            SendMessage( hWnd, msg, wp, lp );
        }
    }
}

bool message_manager::IsAllowedAsyncMessage( UINT msg )
{
    return ( IsInEnumRange<CallbackMessage>( msg )
             || IsInEnumRange<PlayerMessage>( msg )
             || IsInEnumRange<InternalAsyncMessage>( msg ) );
}

void message_manager::post_msg_impl( HWND hWnd, WindowData& windowData, UINT msg, WPARAM wp, LPARAM lp )
{
    auto& [currentGeneration, callbackMsgQueue, asyncMsgQueue, isAsyncEnabled] = windowData;
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

void message_manager::post_callback_msg_impl( HWND hWnd, WindowData& windowData, CallbackMessage msg, std::shared_ptr<CallbackData> msgData )
{
    auto& [currentGeneration, callbackMsgQueue, asyncMsgQueue, isAsyncEnabled] = windowData;
    if ( !isAsyncEnabled )
    {
        return;
    }

    callbackMsgQueue.emplace_back( msg, msgData );
    asyncMsgQueue.emplace_back( static_cast<INT>( msg ), 0, 0 );

    if ( !PostMessage( hWnd, static_cast<INT>( MiscMessage::run_task_async ), currentGeneration, 0 ) )
    {
        asyncMsgQueue.pop_back();
        callbackMsgQueue.pop_back();
    }
}

} // namespace smp::panel
