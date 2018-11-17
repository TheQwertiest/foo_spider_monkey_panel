#pragma once

#include <user_message.h>
#include <callback_data.h>

#include <mutex>
#include <deque>
#include <map>

namespace smp::panel
{

// TODO: consider removing fromhook
// TODO: move to a more suitable place
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

class message_manager
{
public:
    message_manager() = default;
    message_manager( const message_manager& ) = delete;
    message_manager& operator=( const message_manager& ) = delete;

    static message_manager& instance();

    void add_window( HWND hWnd );
    void regenerate_window( HWND hWnd );
    void remove_window( HWND hWnd );

    bool IsAsyncMessageRelevant( HWND hWnd, UINT msg, LPARAM lp );
    std::shared_ptr<CallbackData> ClaimCallbackMessage( HWND hWnd, smp::CallbackMessage msg );

    void post_msg( HWND hWnd, UINT msg, WPARAM wp = 0 );
    void post_msg_to_all( UINT msg, WPARAM wp = 0 );
    void post_callback_msg( HWND hWnd, smp::CallbackMessage msg, std::unique_ptr<smp::panel::CallbackData> data );
    void post_callback_msg_to_all( smp::CallbackMessage msg, std::unique_ptr<smp::panel::CallbackData> data );
    
    void send_msg_to_all( UINT msg, WPARAM wp = 0, LPARAM lp = 0 );
    void send_msg_to_others( HWND hWnd_except, UINT msg, WPARAM wp = 0, LPARAM lp = 0 );

private:
    struct Message
    {
        Message( CallbackMessage id, std::shared_ptr<CallbackData> pData )
            : id( id )
            , pData( pData )
        {
        }
        CallbackMessage id;
        std::shared_ptr<CallbackData> pData;
    };

    struct WindowData
    {
        uint32_t currentGeneration = 0;
        std::deque<Message> msgQueue;
    };

    std::mutex dataMutex_;
    std::map<HWND, WindowData> windowData_;
};

} // namespace smp::panel
