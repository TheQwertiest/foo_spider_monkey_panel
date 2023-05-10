#include <stdafx.h>

#include "mouse_event.h"

namespace smp
{

MouseEventNew::MouseEventNew( EventId id,
                              MouseKeyFlag button,
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

int32_t MouseEventNew::GetX() const
{
    return x_;
}

int32_t MouseEventNew::GetY() const
{
    return y_;
}

MouseEventNew::MouseKeyFlag MouseEventNew::GetButton() const
{
    return button_;
}

MouseEventNew::MouseKeyFlag MouseEventNew::GetButtons() const
{
    return buttons_;
}

MouseEventNew::ModifierKeyFlag MouseEventNew::GetModifiers() const
{
    return modifiers_;
}

MouseEventNew::MouseKeyFlag MouseEventNew::CalculateButtons()
{
    auto buttons = MouseKeyFlag::kNoButtons;

    if ( ::GetKeyState( VK_LBUTTON ) < 0 )
    {
        buttons |= MouseKeyFlag::kPrimary;
    }
    if ( ::GetKeyState( VK_RBUTTON ) < 0 )
    {
        buttons |= MouseKeyFlag::kSecondary;
    }
    if ( ::GetKeyState( VK_MBUTTON ) < 0 )
    {
        buttons |= MouseKeyFlag::kAuxiliary;
    }
    if ( ::GetKeyState( VK_XBUTTON1 ) < 0 )
    {
        buttons |= MouseKeyFlag::k4;
    }
    if ( ::GetKeyState( VK_XBUTTON2 ) < 0 )
    {
        buttons |= MouseKeyFlag::k5;
    }

    return buttons;
}

MouseEventNew::ModifierKeyFlag MouseEventNew::CalculateModifiers()
{
    auto modifiers = ModifierKeyFlag::kNoModifiers;

    if ( ::GetKeyState( VK_MENU ) < 0 )
    {
        modifiers |= ModifierKeyFlag::kAlt;
    }
    if ( ::GetKeyState( VK_CONTROL ) < 0 )
    {
        modifiers |= ModifierKeyFlag::kCtrl;
    }
    if ( ::GetKeyState( VK_SHIFT ) < 0 )
    {
        modifiers |= ModifierKeyFlag::kShift;
    }
    if ( ::GetKeyState( VK_LWIN ) < 0 || ::GetKeyState( VK_RWIN ) < 0 )
    {
        modifiers |= ModifierKeyFlag::kWin;
    }

    return modifiers;
}

} // namespace smp
