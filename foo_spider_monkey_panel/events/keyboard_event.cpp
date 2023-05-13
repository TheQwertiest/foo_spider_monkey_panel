#include <stdafx.h>

#include "keyboard_event.h"

namespace smp
{

KeyboardEvent::KeyboardEvent( EventId id,
                              const qwr::u8string& key,
                              uint32_t code )
    : PanelEvent( id )
    , key_( key )
    , code_( code )
{
}

const qwr::u8string& KeyboardEvent::GetKey() const
{
    return key_;
}

uint32_t KeyboardEvent::GetCode() const
{
    return code_;
}

} // namespace smp
