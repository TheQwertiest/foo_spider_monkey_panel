#pragma once

#include <panel/user_message.h>

#include <mutex>
#include <optional>
#include <unordered_set>

namespace smp::panel
{

class MessageManager
{
public:
    MessageManager() = default;
    MessageManager( const MessageManager& ) = delete;
    MessageManager& operator=( const MessageManager& ) = delete;

    static MessageManager& Get();

public:
    void AddWindow( HWND hWnd );
    void RemoveWindow( HWND hWnd );

public:
    void SendMsgToAll( UINT msg, WPARAM wp = 0, LPARAM lp = 0 );
    void SendMsgToOthers( HWND hWnd_except, UINT msg, WPARAM wp = 0, LPARAM lp = 0 );

private:
    std::mutex wndDataMutex_;
    std::unordered_set<HWND> wndDataMap_;
};

} // namespace smp::panel
