#include <stdafx.h>

#include "event_drag.h"

#include <js_engine/js_container.h>
#include <panel/js_panel_window.h>

namespace smp
{

Event_Drag::Event_Drag( EventId id, int32_t x, int32_t y, uint32_t mask, uint32_t modifiers, const panel::DragActionParams& dragParams )
    : Event_Mouse( id, x, y, mask, modifiers )
    , dragParams_( dragParams )
{
}

Event_Drag* Event_Drag::AsDragEvent()
{
    return this;
}

std::optional<bool> Event_Drag::JsExecute( mozjs::JsContainer& /*jsContainer*/ )
{
    assert( false );
    return std::nullopt;
}

const panel::DragActionParams& Event_Drag::GetDragParams() const
{
    return dragParams_;
}

} // namespace smp
