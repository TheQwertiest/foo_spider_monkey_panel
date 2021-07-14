#include <stdafx.h>

#include "event_mouse.h"

#include <js_engine/js_container.h>
#include <panel/js_panel_window.h>

namespace smp
{

Event_Mouse::Event_Mouse( EventId id, int32_t x, int32_t y, uint32_t mask )
    : id_( id )
    , x_( x )
    , y_( y )
    , mask_( mask )
{
    assert( kCallbackIdToName.count( id_ ) );
}

void Event_Mouse::Run( panel::js_panel_window& panelWindow )
{
    assert( core_api::is_main_thread() );
    panelWindow.ExecuteJsTask( id_, *this );
}

std::optional<bool> Event_Mouse::JsExecute( mozjs::JsContainer& jsContainer )
{
    const auto callbackName = fmt::format( "on_{}", kCallbackIdToName.at( id_ ) );
    return jsContainer.InvokeJsCallback<bool>( callbackName, x_, y_, mask_ );
}

Event_Mouse* Event_Mouse::AsMouseEvent()
{
    return this;
}

int32_t Event_Mouse::GetX() const
{
    return x_;
}

int32_t Event_Mouse::GetY() const
{
    return y_;
}

uint32_t Event_Mouse::GetMask() const
{
    return mask_;
}

} // namespace smp
