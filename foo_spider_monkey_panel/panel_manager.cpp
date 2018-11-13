#include <stdafx.h>
#include "panel_manager.h"

#include <message_data_holder.h>


namespace smp::panel
{

panel::panel_manager panel::panel_manager::sm_instance;

panel::panel_manager& panel::panel_manager::instance()
{
    return sm_instance;
}

t_size panel::panel_manager::get_count()
{
    return m_hwnds.size();
}

void panel::panel_manager::add_window( HWND p_wnd )
{
    if ( m_hwnds.cend() == std::find( m_hwnds.cbegin(), m_hwnds.cend(), p_wnd ) )
    {
        m_hwnds.push_back( p_wnd );
    }
}

void panel::panel_manager::post_msg_to_all( UINT p_msg )
{
    post_msg_to_all( p_msg, 0, 0 );
}

void panel::panel_manager::post_msg_to_all( UINT p_msg, WPARAM p_wp )
{
    post_msg_to_all( p_msg, p_wp, 0 );
}

void panel::panel_manager::post_msg_to_all( UINT p_msg, WPARAM p_wp, LPARAM p_lp )
{
    for ( const auto& hWnd : m_hwnds )
    {
        PostMessage( hWnd, p_msg, p_wp, p_lp );
    }
}

void panel::panel_manager::post_callback_msg( HWND p_wnd, smp::CallbackMessage p_msg, std::unique_ptr<CallbackData> data )
{
    auto& dataStorage = MessageDataHolder::GetInstance();

    std::shared_ptr<panel::CallbackData> sharedData( data.release() );
    dataStorage.StoreData( p_msg, std::vector<HWND>{ p_wnd }, sharedData );

    BOOL bRet = PostMessage( p_wnd, static_cast<UINT>( p_msg ), 0, 0 );
    if ( !bRet )
    {
        dataStorage.FlushDataForHwnd( p_wnd, sharedData.get() );
    }
}

void panel::panel_manager::post_callback_msg_to_all( CallbackMessage p_msg, std::unique_ptr<panel::CallbackData> data )
{
    if ( m_hwnds.empty() )
    {
        return;
    }

    auto& dataStorage = MessageDataHolder::GetInstance();

    std::shared_ptr<panel::CallbackData> sharedData( data.release() );
    dataStorage.StoreData( p_msg, m_hwnds, sharedData );

    for ( const auto& hWnd : m_hwnds )
    {
        BOOL bRet = PostMessage( hWnd, static_cast<UINT>( p_msg ), 0, 0 );
        if ( !bRet )
        {
            dataStorage.FlushDataForHwnd( hWnd, sharedData.get() );
        }
    }
}

void panel::panel_manager::remove_window( HWND p_wnd )
{
    if ( auto it = std::find( m_hwnds.cbegin(), m_hwnds.cend(), p_wnd ); m_hwnds.cend() != it )
    {
        m_hwnds.erase( it );
    }
}

void panel::panel_manager::send_msg_to_all( UINT p_msg, WPARAM p_wp, LPARAM p_lp )
{
    for ( const auto& hWnd : m_hwnds )
    {
        SendMessage( hWnd, p_msg, p_wp, p_lp );
    }
}

void panel::panel_manager::send_msg_to_others( HWND p_wnd_except, UINT p_msg, WPARAM p_wp, LPARAM p_lp )
{
    if ( m_hwnds.size() < 2 )
    {
        return;
    }

    for ( const auto& hWnd : m_hwnds )
    {
        if ( hWnd != p_wnd_except )
        {
            SendMessage( hWnd, p_msg, p_wp, p_lp );
        }
    }
}

} // namespace smp::panel
