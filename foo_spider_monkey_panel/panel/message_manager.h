#pragma once

#include <panel/callback_data.h>
#include <panel/user_message.h>

#include <deque>
#include <map>
#include <mutex>
#include <optional>

namespace smp::panel
{

class MessageManager
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
            , pData( std::move( pData ) )
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
    MessageManager() = default;
    MessageManager( const MessageManager& ) = delete;
    MessageManager& operator=( const MessageManager& ) = delete;

    static MessageManager& Get();

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
    void PostMsg( HWND hWnd, UINT msg, WPARAM wp = 0, LPARAM lp = 0 );
    void PostMsgToAll( UINT msg, WPARAM wp = 0, LPARAM lp = 0 );
    void PostCallbackMsg( HWND hWnd, smp::CallbackMessage msg, std::unique_ptr<smp::panel::CallbackData> data );
    void PostCallbackMsgToAll( smp::CallbackMessage msg, std::unique_ptr<smp::panel::CallbackData> data );

    void SendMsgToAll( UINT msg, WPARAM wp = 0, LPARAM lp = 0 );
    void SendMsgToOthers( HWND hWnd_except, UINT msg, WPARAM wp = 0, LPARAM lp = 0 );

private:
    static bool IsAllowedAsyncMessage( UINT msg );
    static void PostMsgImpl( HWND hWnd, WindowData& windowData, UINT msg, WPARAM wp, LPARAM lp );
    static void PostCallbackMsgImpl( HWND hWnd, WindowData& windowData, CallbackMessage msg, std::shared_ptr<CallbackData> msgData );

private:
    std::mutex wndDataMutex_;
    std::map<HWND, WindowData> wndDataMap_;
};

} // namespace smp::panel
