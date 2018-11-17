#include <stdafx.h>
#include "message_manager.h"


namespace smp::panel
{

message_manager& message_manager::instance()
{
    static message_manager sm_instance;
    return sm_instance;
}

void message_manager::add_window( HWND hWnd )
{
    std::scoped_lock sl( dataMutex_ );

    assert( !windowData_.count( hWnd ) );
    windowData_.emplace( hWnd, WindowData{} );
}

void message_manager::regenerate_window( HWND hWnd )
{
    std::scoped_lock sl( dataMutex_ );

    assert( windowData_.count( hWnd ) );
    ++(windowData_[hWnd].currentGeneration);
    windowData_[hWnd].msgQueue.clear();
}

void message_manager::remove_window( HWND hWnd )
{
    std::scoped_lock sl( dataMutex_ );

    assert( windowData_.count( hWnd ) );
    windowData_.erase( hWnd );
}

bool message_manager::IsAsyncMessageRelevant( HWND hWnd, UINT msg, LPARAM lp )
{
    std::scoped_lock sl( dataMutex_ );

    assert( windowData_.count( hWnd ) );
    return windowData_[hWnd].currentGeneration == static_cast<uint32_t>( lp );
}

std::shared_ptr<CallbackData> message_manager::ClaimCallbackMessage( HWND hWnd, CallbackMessage msg )
{
    std::scoped_lock sl( dataMutex_ );

    assert( windowData_.count( hWnd ) );
    auto& [currentGeneration, msgQueue] = windowData_[hWnd];

    const auto it = std::find_if( msgQueue.cbegin(), msgQueue.cend(), [msgId = msg]( const auto& msg ) {
        return msg.id == msgId;
    } );
    assert( it != msgQueue.cend() );

    auto msgData = it->pData;
    msgQueue.erase( it );

    return msgData;
}

void message_manager::post_msg( HWND hWnd, UINT msg, WPARAM wp )
{
    std::scoped_lock sl( dataMutex_ );

    assert( windowData_.count( hWnd ) );
    PostMessage( hWnd, msg, wp, windowData_[hWnd].currentGeneration );
}

void message_manager::post_msg_to_all( UINT msg, WPARAM wp )
{
    std::scoped_lock sl( dataMutex_ );
    for ( const auto& [hWnd, wndData] : windowData_ )
    {
        PostMessage( hWnd, msg, wp, wndData.currentGeneration );
    }
}

void message_manager::post_callback_msg( HWND hWnd, CallbackMessage msg, std::unique_ptr<CallbackData> data )
{
    std::shared_ptr<CallbackData> sharedData( data.release() );

    std::scoped_lock sl( dataMutex_ );
    assert( windowData_.count( hWnd ) );

    auto& [currentGeneration, msgQueue] = windowData_[hWnd];

    msgQueue.emplace_back( msg, sharedData );

    BOOL bRet = PostMessage( hWnd, static_cast<UINT>( msg ), 0, currentGeneration );
    if ( !bRet )
    {
        msgQueue.pop_back();
    }
}

void message_manager::post_callback_msg_to_all( CallbackMessage msg, std::unique_ptr<CallbackData> data )
{
    if ( windowData_.empty() )
    {
        return;
    }

    std::shared_ptr<CallbackData> sharedData( data.release() );

    std::scoped_lock sl( dataMutex_ );
    for ( auto& [hWnd, wndData] : windowData_ )
    {
        wndData.msgQueue.emplace_back( msg, sharedData );

        BOOL bRet = PostMessage( hWnd, static_cast<UINT>( msg ), 0, wndData.currentGeneration );
        if ( !bRet )
        {
            wndData.msgQueue.pop_back();
        }
    }
}

void message_manager::send_msg_to_all( UINT msg, WPARAM wp, LPARAM lp )
{
    std::vector<HWND> hWnds;
    hWnds.reserve( windowData_.size() );

    {
        std::scoped_lock sl( dataMutex_ );
        for ( const auto& [hWnd, wndData] : windowData_ )
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
    hWnds.reserve( windowData_.size() );

    {
        std::scoped_lock sl( dataMutex_ );
        for ( const auto& [hWnd, wndData] : windowData_ )
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

} // namespace smp::panel
