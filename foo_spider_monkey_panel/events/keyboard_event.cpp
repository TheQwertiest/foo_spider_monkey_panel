#include <stdafx.h>

#include "keyboard_event.h"

namespace smp
{

KeyboardEvent::KeyboardEvent( EventId id,
                              const std::wstring& chars,
                              uint32_t virtualCode,
                              uint32_t scanCode,
                              bool isExtendedScanCode,
                              bool isRepeating )
    : PanelEvent( id )
    , chars_( chars )
    , virtualCode_( virtualCode )
    , scanCode_( scanCode )
    , isExtendedScanCode_( isExtendedScanCode )
    , isRepeating_( isRepeating )
{
}

const std::wstring& KeyboardEvent::GetChars() const
{
    return chars_;
}

uint32_t KeyboardEvent::GetVirtualCode() const
{
    return virtualCode_;
}

uint32_t KeyboardEvent::GetScanCode() const
{
    return scanCode_;
}

bool KeyboardEvent::IsExtendedScanCode() const
{
    return isExtendedScanCode_;
}

uint32_t KeyboardEvent::GetFullScanCode() const
{
    return ( isExtendedScanCode_ ? scanCode_ | 0xE000 : scanCode_ );
}

KeyboardEvent::ModifierFlag KeyboardEvent::GetModifiers() const
{
    return modifiers_;
}

bool KeyboardEvent::IsRepeating() const
{
    return isRepeating_;
}

KeyboardEvent::KeyLocation KeyboardEvent::GetKeyLocation() const
{
    switch ( virtualCode_ )
    {
    case VK_LCONTROL:
    case VK_LMENU:
    case VK_LSHIFT:
    case VK_LWIN:
        return KeyLocation::kLeft;
    case VK_RCONTROL:
    case VK_RMENU:
    case VK_RSHIFT:
    case VK_RWIN:
        return KeyLocation::kRight;
    case VK_RETURN:
        return ( isExtendedScanCode_ ? KeyLocation::kNumpad : KeyLocation::kStandard );
    case VK_CLEAR:
    case VK_DELETE:
    case VK_DOWN:
    case VK_END:
    case VK_HOME:
    case VK_INSERT:
    case VK_LEFT:
    case VK_NEXT:
    case VK_PRIOR:
    case VK_RIGHT:
    case VK_UP:
        return ( isExtendedScanCode_ ? KeyLocation::kNumpad : KeyLocation::kStandard );
    case VK_ADD:
    case VK_DECIMAL:
    case VK_DIVIDE:
    case VK_MULTIPLY:
    case VK_NUMPAD0:
    case VK_NUMPAD1:
    case VK_NUMPAD2:
    case VK_NUMPAD3:
    case VK_NUMPAD4:
    case VK_NUMPAD5:
    case VK_NUMPAD6:
    case VK_NUMPAD7:
    case VK_NUMPAD8:
    case VK_NUMPAD9:
    case VK_SUBTRACT:
        return KeyLocation::kNumpad;
    default:
        return KeyLocation::kStandard;
    }
}

KeyboardEvent::ModifierFlag KeyboardEvent::CalculateModifiers()
{
    auto modifiers = ModifierFlag::kNoModifiers;

    if ( ::GetKeyState( VK_MENU ) < 0 )
    {
        modifiers |= ModifierFlag::kAlt;
    }
    if ( ::GetKeyState( VK_CONTROL ) < 0 )
    {
        modifiers |= ModifierFlag::kCtrl;
    }
    if ( ::GetKeyState( VK_SHIFT ) < 0 )
    {
        modifiers |= ModifierFlag::kShift;
    }
    if ( ::GetKeyState( VK_LWIN ) < 0 || ::GetKeyState( VK_RWIN ) < 0 )
    {
        modifiers |= ModifierFlag::kWin;
    }

    return modifiers;
}

} // namespace smp
