#include <stdafx.h>

#include "hook_handler.h"

#include <utils/winapi_error_helpers.h>

namespace smp::utils
{

HookHandler::~HookHandler()
{
    assert( callbacks_.empty() );
    if ( hHook_ )
    {
        ::UnhookWindowsHookEx( hHook_ );
    }
}

HookHandler& HookHandler::GetInstance()
{
    static HookHandler hh;
    return hh;
}

void HookHandler::UnregisterHook( uint32_t hookId )
{
    assert( callbacks_.count( hookId ) );
    callbacks_.erase( hookId );
}

void HookHandler::MaybeRegisterGlobalHook()
{
    if ( !hHook_ )
    {
        hHook_ = ::SetWindowsHookEx( WH_GETMESSAGE, GetMsgProc, nullptr, ::GetCurrentThreadId() );
        smp::error::CheckWinApi( hHook_, "SetWindowsHookEx" );
    }
}

LRESULT CALLBACK HookHandler::GetMsgProc( int code, WPARAM wParam, LPARAM lParam )
{
    for ( auto it = callbacks_.begin(); it != callbacks_.end(); )
    {
        // callback might trigger self-destruction, thus we need to preserve it
        auto tmpCallback = *it->second;
        ++it;
        std::invoke( tmpCallback, code, wParam, lParam );
    }

    return CallNextHookEx( nullptr, code, wParam, lParam );
}

std::unordered_map<uint32_t, std::shared_ptr<HookHandler::HookCallback>> HookHandler::callbacks_;

} // namespace smp::utils
