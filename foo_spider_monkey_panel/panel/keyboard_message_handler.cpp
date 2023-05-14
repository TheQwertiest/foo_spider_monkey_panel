#include <stdafx.h>

#include "keyboard_message_handler.h"

#include <events/dispatcher/event_dispatcher.h>
#include <events/keyboard_event.h>
#include <panel/panel_window.h>

#include <cwctype>

namespace
{

auto GetScanCodeFromMessage( const MSG& msg )
{
    const uint32_t scanCode = ( ( msg.lParam >> 16 ) & 0xFF );
    const bool isExtendedScanCode = !!( msg.lParam & 0x1000000 );
    return std::make_tuple( scanCode, isExtendedScanCode );
}

auto GetVirtualCodeFromMessage( const MSG& msg )
{
    assert( msg.message != WM_CHAR );
    return static_cast<uint32_t>( msg.wParam );
}

std::optional<uint32_t> GetVirtualCodeFromScanCode( uint32_t scanCode, bool isExtendedScanCode )
{
    const auto iRet = ::MapVirtualKeyEx( isExtendedScanCode ? ( 0xE000 | scanCode ) : scanCode, MAPVK_VSC_TO_VK_EX, ::GetKeyboardLayout( 0 ) );
    if ( !iRet )
    {
        return std::nullopt;
    }

    return static_cast<uint32_t>( iRet );
}

std::optional<std::wstring> GetCharsFromMessage( const MSG& msg )
{
    assert( msg.message == WM_CHAR );
    if ( std::iswcntrl( static_cast<wint_t>( msg.wParam ) ) )
    {
        return std::nullopt;
    }

    return std::wstring( 1, static_cast<wchar_t>( msg.wParam ) );
}

std::optional<std::wstring> GetCharsFromCodes( uint32_t virtualCode, uint32_t scanCode )
{
    // TODO: consider caching keyboard layout to avoid ToUnicodeEx calls
    std::array<BYTE, 256> keyboardState{};
    keyboardState[VK_SHIFT] = static_cast<BYTE>( ::GetKeyState( VK_SHIFT ) );
    keyboardState[VK_CAPITAL] = static_cast<BYTE>( ::GetKeyState( VK_CAPITAL ) );

    std::array<wchar_t, 1> charBuffer{};
    int iRet = ::ToUnicodeEx( virtualCode,
                              scanCode,
                              keyboardState.data(),
                              charBuffer.data(),
                              charBuffer.size(),
                              0,
                              ::GetKeyboardLayout( 0 ) );
    if ( !iRet || std::iswcntrl( static_cast<wint_t>( charBuffer[0] ) ) )
    {
        return std::nullopt;
    }

    return std::wstring{ charBuffer.data(), charBuffer.size() };
}

} // namespace

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
    // WM_SYSKEY* handles special keys (e.g. ALT)
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYUP:
    {
        const auto [scanCode, isExtendedScanCode] = GetScanCodeFromMessage( msg );
        const auto virtualCode = GetVirtualCodeFromMessage( msg );

        std::optional<std::wstring> charsOpt;
        MSG peekMsg;
        // needed for *KEYDOWN in case a displayable character is entered,
        // needed for *KEYUP in case character was entered via ALT+NUMPAD
        bool hasMessage = PeekMessage( &peekMsg, wnd, WM_CHAR, WM_CHAR, PM_NOREMOVE | PM_NOYIELD );
        if ( hasMessage )
        {
            MSG newMsg;
            hasMessage = PeekMessage( &newMsg, peekMsg.hwnd, peekMsg.message, peekMsg.message, PM_REMOVE | PM_NOYIELD );
            if ( hasMessage )
            {
                charsOpt = GetCharsFromMessage( newMsg );
            }
        }
        if ( !charsOpt )
        {
            charsOpt = GetCharsFromCodes( virtualCode, scanCode );
        }

        const auto eventId = ( msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN
                                   ? EventId::kNew_KeyboardKeyDown
                                   : EventId::kNew_KeyboardKeyUp );
        EventDispatcher::Get().PutEvent( wnd,
                                         std::make_unique<KeyboardEvent>(
                                             eventId,
                                             charsOpt.value_or( L"" ),
                                             virtualCode,
                                             scanCode,
                                             isExtendedScanCode ),
                                         EventPriority::kInput );
        return 0;
    }
    // TODO: handle UNICHAR and DEADCHAR (if it's actually needed)
    case WM_CHAR:
    {
        const auto [scanCode, isExtendedScanCode] = GetScanCodeFromMessage( msg );
        const auto virtualCodeOpt = GetVirtualCodeFromScanCode( scanCode, isExtendedScanCode );

        const auto charsOpt = GetCharsFromMessage( msg );
        if ( !charsOpt )
        {
            return 0;
        }

        // const auto wasKeyPressed = !!( msg.lParam & ( 1 << 30 ) );
        EventDispatcher::Get().PutEvent( wnd,
                                         std::make_unique<KeyboardEvent>(
                                             EventId::kNew_KeyboardKeyDown,
                                             *charsOpt,
                                             virtualCodeOpt.value_or( 0 ),
                                             scanCode,
                                             isExtendedScanCode ),
                                         EventPriority::kInput );

        return 0;
    }
    default:
    {
        assert( false );
        return std::nullopt;
    }
    }
}

} // namespace smp::panel
