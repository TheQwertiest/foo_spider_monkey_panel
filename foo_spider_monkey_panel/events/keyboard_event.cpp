#include <stdafx.h>

#include "keyboard_event.h"

namespace smp
{

KeyboardEvent::KeyboardEvent( EventId id,
                              const std::wstring& chars,
                              uint32_t virtualCode,
                              uint32_t scanCode,
                              bool isExtendedScanCode )
    : PanelEvent( id )
    , chars_( chars )
    , virtualCode_( virtualCode )
    , scanCode_( scanCode )
    , isExtendedScanCode_( isExtendedScanCode )
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

} // namespace smp
