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
    wndDataMap_.emplace( hWnd );
}

void MessageManager::RemoveWindow( HWND hWnd )
{
    std::scoped_lock sl( wndDataMutex_ );

    assert( wndDataMap_.contains( hWnd ) );
    wndDataMap_.erase( hWnd );
}

void MessageManager::SendMsgToAll( UINT msg, WPARAM wp, LPARAM lp )
{
    std::vector<HWND> hWnds;
    hWnds.reserve( wndDataMap_.size() );

    {
        std::scoped_lock sl( wndDataMutex_ );
        for ( const auto& hWnd: wndDataMap_ )
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
        for ( const auto& hWnd: wndDataMap_ )
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

} // namespace smp::panel
