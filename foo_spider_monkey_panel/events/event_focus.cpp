#include <stdafx.h>

#include "event_focus.h"

#include <js_engine/js_container.h>
#include <panel/js_panel_window.h>

namespace smp
{

Event_Focus::Event_Focus( EventId id, bool isFocused )
    : id_( id )
    , isFocused_( isFocused )
{
    assert( kCallbackIdToName.count( id_ ) );
}

void Event_Focus::Run( panel::js_panel_window& panelWindow )
{
    assert( core_api::is_main_thread() );
    panelWindow.ExecuteJsTask( id_, *this );
}

std::optional<bool> Event_Focus::JsExecute( mozjs::JsContainer& jsContainer )
{
    const auto callbackName = fmt::format( "on_{}", kCallbackIdToName.at( id_ ) );
    return jsContainer.InvokeJsCallback<bool>( callbackName, isFocused_ );
}

Event_Focus* Event_Focus::AsFocusEvent()
{
    return this;
}

bool Event_Focus::IsFocused() const
{
    return isFocused_;
}

} // namespace smp
