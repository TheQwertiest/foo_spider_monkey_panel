#pragma once
#include "helpers.h"

#include <user_message.h>
#include <callback_data.h>

namespace smp::panel
{

// TODO: consider removing fromhook
struct metadb_callback_data
{
    metadb_handle_list m_items;
    bool m_fromhook;

    metadb_callback_data( const metadb_handle_list& p_items, bool p_fromhook )
        : m_items( p_items )
        , m_fromhook( p_fromhook )
    {
    }
};

class panel_manager
{
public:
    panel_manager() = default;
    panel_manager( const panel_manager& ) = delete;
    panel_manager& operator=( const panel_manager& ) = delete;

    static panel_manager& instance();

    t_size get_count();
    void add_window( HWND p_wnd );

    void post_msg_to_all( UINT p_msg );
    void post_msg_to_all( UINT p_msg, WPARAM p_wp );
    void post_msg_to_all( UINT p_msg, WPARAM p_wp, LPARAM p_lp );
    void post_callback_msg( HWND p_wnd, smp::CallbackMessage p_msg, std::unique_ptr<smp::panel::CallbackData> data );
    void post_callback_msg_to_all( smp::CallbackMessage p_msg, std::unique_ptr<smp::panel::CallbackData> data );

    void remove_window( HWND p_wnd );
    void send_msg_to_all( UINT p_msg, WPARAM p_wp, LPARAM p_lp );
    void send_msg_to_others( HWND p_wnd_except, UINT p_msg, WPARAM p_wp, LPARAM p_lp );

private:
    std::vector<HWND> m_hwnds;
    static panel_manager sm_instance;
};

} // namespace smp::panel
