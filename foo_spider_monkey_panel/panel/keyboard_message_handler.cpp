#include <stdafx.h>

#include "keyboard_message_handler.h"

#include <events/dispatcher/event_dispatcher.h>
#include <events/keyboard_event.h>
#include <panel/panel_window.h>

namespace smp::panel
{

KeyboardMessageHandler::KeyboardMessageHandler( PanelWindow& parent )
    : parent_( parent )
{
}

std::optional<LRESULT> KeyboardMessageHandler::HandleMessage( const MSG& msg )
{
    CWindow wnd = parent_.GetHWND();

    switch ( msg.message )
    {
    // WM_SYSKEYDOWN handles special keys (e.g. ALT)
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
    {
        qwr::u8string key{ "system" };

        MSG peekMsg;
        bool hasMessage = PeekMessage( &peekMsg, wnd, WM_CHAR, WM_CHAR, PM_NOREMOVE | PM_NOYIELD );
        if ( hasMessage )
        {
            MSG newMsg;
            hasMessage = PeekMessage( &newMsg, peekMsg.hwnd, peekMsg.message, peekMsg.message, PM_REMOVE | PM_NOYIELD );
            key = ( hasMessage ? qwr::unicode::ToU8( std::wstring( 1, static_cast<wchar_t>( peekMsg.wParam ) ) ) : "system" );
        }
        const auto virtualCode = static_cast<uint32_t>( msg.wParam );

        EventDispatcher::Get().PutEvent( wnd,
                                         std::make_unique<KeyboardEvent>(
                                             EventId::kNew_KeyboardKeyDown,
                                             key,
                                             virtualCode ),
                                         EventPriority::kInput );
        return 0;
    }
    // TODO: handle UNICHAR and DEADCHAR (if it's actually needed)
    case WM_CHAR:
    {
        const auto key = qwr::unicode::ToU8( std::wstring( 1, static_cast<wchar_t>( msg.wParam ) ) );

        const auto scanCode = ( ( msg.lParam >> 16 ) & 0xFF );
        const auto isExtendedScanCode = !!( msg.lParam & 0x1000000 );
        const auto virtualCode = ::MapVirtualKeyEx( isExtendedScanCode ? ( 0xE000 | scanCode ) : scanCode, MAPVK_VSC_TO_VK_EX, ::GetKeyboardLayout( 0 ) );

        const auto wasKeyPressed = !!( msg.lParam & ( 1 << 30 ) );
        EventDispatcher::Get().PutEvent( wnd,
                                         std::make_unique<KeyboardEvent>(
                                             EventId::kNew_KeyboardKeyDown,
                                             key,
                                             virtualCode ),
                                         EventPriority::kInput );

        return 0;
    }
    // WM_SYSKEYUP handles special keys (e.g. ALT)
    case WM_SYSKEYUP:
    case WM_KEYUP:
    {
        const auto virtualCode = static_cast<uint32_t>( msg.wParam );

        const auto scanCode = ( ( msg.lParam >> 16 ) & 0xFF );
        const auto isExtendedScanCode = !!( msg.lParam & 0x1000000 );

        BYTE keyboardState[256] = { 0 };
        wchar_t charBuffer[1] = { 0 };
        int iRet =
            ::ToUnicodeEx( virtualCode, scanCode, keyboardState, charBuffer, std::size( charBuffer ), 0, ::GetKeyboardLayout( 0 ) );
        const auto key = ( iRet == 1 ? qwr::unicode::ToU8( std::wstring( 1, charBuffer[0] ) ) : "system" );

        EventDispatcher::Get().PutEvent( wnd,
                                         std::make_unique<KeyboardEvent>(
                                             EventId::kNew_KeyboardKeyUp,
                                             key,
                                             virtualCode ),
                                         EventPriority::kInput );
        return 0;
    }
    case WM_INPUTLANGCHANGE: // TODO:
    default:
    {
        assert( false );
        return std::nullopt;
    }
    }
}

} // namespace smp::panel
