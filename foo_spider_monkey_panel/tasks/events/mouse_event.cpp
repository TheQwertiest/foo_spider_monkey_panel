#include <stdafx.h>

#include "mouse_event.h"

namespace smp
{

MouseEvent::MouseEvent( EventId id,
                        KeyFlag button,
                        int32_t x,
                        int32_t y )
    : PanelEvent( id )
    , x_( x )
    , y_( y )
    , button_( button )
    , buttons_( CalculateButtons() )
    , modifiers_( CalculateModifiers() )
{
}

int32_t MouseEvent::GetX() const
{
    return x_;
}

int32_t MouseEvent::GetY() const
{
    return y_;
}

MouseEvent::KeyFlag MouseEvent::GetButton() const
{
    return button_;
}

MouseEvent::KeyFlag MouseEvent::GetButtons() const
{
    return buttons_;
}

MouseEvent::ModifierFlag MouseEvent::GetModifiers() const
{
    return modifiers_;
}

MouseEvent::KeyFlag MouseEvent::CalculateButtons()
{
    auto buttons = KeyFlag::kNoButtons;

    if ( ::GetKeyState( VK_LBUTTON ) < 0 )
    {
        buttons |= KeyFlag::kPrimary;
    }
    if ( ::GetKeyState( VK_RBUTTON ) < 0 )
    {
        buttons |= KeyFlag::kSecondary;
    }
    if ( ::GetKeyState( VK_MBUTTON ) < 0 )
    {
        buttons |= KeyFlag::kAuxiliary;
    }
    if ( ::GetKeyState( VK_XBUTTON1 ) < 0 )
    {
        buttons |= KeyFlag::k4;
    }
    if ( ::GetKeyState( VK_XBUTTON2 ) < 0 )
    {
        buttons |= KeyFlag::k5;
    }

    return buttons;
}

MouseEvent::ModifierFlag MouseEvent::CalculateModifiers()
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
