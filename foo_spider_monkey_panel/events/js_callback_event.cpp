#include <stdafx.h>

#include "js_callback_event.h"

#include <js_backend/engine/js_container.h>

namespace smp
{

JsCallbackEventNew::JsCallbackEventNew( EventId id )
    : Event_JsExecutor( id )
{
}

std::unique_ptr<EventBase> JsCallbackEventNew::Clone()
{
    return std::make_unique<JsCallbackEventNew>( id_ );
}

std::optional<bool> JsCallbackEventNew::JsExecute( mozjs::JsContainer& jsContainer )
{
    jsContainer.InvokeJsEvent( *this );
    return std::nullopt;
}

} // namespace smp
