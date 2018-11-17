#pragma once

#include <user_message.h>
#include <callback_data.h>

#include <mutex>
#include <deque>
#include <optional>
#include <map>

namespace smp::panel
{

class message_manager
{
public:
    struct AsyncMessage
    {
        AsyncMessage( UINT id, uint32_t wp = 0, uint32_t lp = 0 )
            : id( id )
            , wp( wp )
            , lp( lp )
        {
        }
        UINT id;
        uint32_t wp;
        uint32_t lp;
    };

private:
    struct CallbackMessageWrap
    {
        CallbackMessageWrap( CallbackMessage id, std::shared_ptr<CallbackData> pData )
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
        std::deque<CallbackMessageWrap> callbackMsgQueue;
        std::deque<AsyncMessage> asyncMsgQueue;
        bool isAsyncEnabled = false;
    };

public:
    message_manager() = default;
    message_manager( const message_manager& ) = delete;
    message_manager& operator=( const message_manager& ) = delete;

    static message_manager& instance();

public:
    void AddWindow( HWND hWnd );
    void RemoveWindow( HWND hWnd );
    void EnableAsyncMessages( HWND hWnd );
    void DisableAsyncMessages( HWND hWnd );

public:
    static bool IsAsyncMessage( UINT msg );
    std::optional<AsyncMessage> ClaimAsyncMessage( HWND hWnd, UINT msg, WPARAM wp, LPARAM lp );
    std::shared_ptr<CallbackData> ClaimCallbackMessageData( HWND hWnd, smp::CallbackMessage msg );
    void RequestNextAsyncMessage( HWND hWnd );

public:
    void post_msg( HWND hWnd, UINT msg, WPARAM wp = 0, LPARAM lp = 0 );
    void post_msg_to_all( UINT msg, WPARAM wp = 0, LPARAM lp = 0 );
    void post_callback_msg( HWND hWnd, smp::CallbackMessage msg, std::unique_ptr<smp::panel::CallbackData> data );
    void post_callback_msg_to_all( smp::CallbackMessage msg, std::unique_ptr<smp::panel::CallbackData> data );

    void send_msg_to_all( UINT msg, WPARAM wp = 0, LPARAM lp = 0 );
    void send_msg_to_others( HWND hWnd_except, UINT msg, WPARAM wp = 0, LPARAM lp = 0 );

private:
    static bool IsAllowedAsyncMessage( UINT msg );
    static void post_msg_impl( HWND hWnd, WindowData& windowData, UINT msg, WPARAM wp, LPARAM lp );
    static void post_callback_msg_impl( HWND hWnd, WindowData& windowData, CallbackMessage msg, std::shared_ptr<CallbackData> msgData );

private:
    std::mutex wndDataMutex_;
    std::map<HWND, WindowData> wndDataMap_;
};

} // namespace smp::panel
