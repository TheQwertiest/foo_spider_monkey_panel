#include <stdafx.h>

#include "event_notify_others.h"

#include <js_engine/js_container.h>
#include <panel/js_panel_window.h>

namespace smp
{

Event_NotifyOthers::Event_NotifyOthers( JSContext* cx, const std::wstring& name, JS::HandleValue info )
    : Event_JsExecutor( EventId::kNotifyOthers )
    , pJsCtx_( cx )
    , heapHelper_( cx )
    , name_( name )
    , jsInfoId_( heapHelper_.Store( info ) )
{
}

Event_NotifyOthers::~Event_NotifyOthers()
{
    heapHelper_.Finalize();
}

std::unique_ptr<EventBase> Event_NotifyOthers::Clone()
{
    // Note: this clone operation might result in JS object being used in another panel/global
    assert( heapHelper_.IsJsAvailable() );

    JS::RootedValue jsValue( pJsCtx_, heapHelper_.Get( jsInfoId_ ) );
    return std::make_unique<Event_NotifyOthers>( pJsCtx_, name_, jsValue );
}

std::optional<bool> Event_NotifyOthers::JsExecute( mozjs::JsContainer& jsContainer )
{
    if ( !heapHelper_.IsJsAvailable() )
    { // This might happen if the original panel/global died
        return std::nullopt;
    }

    JS::RootedValue jsValue( pJsCtx_, heapHelper_.Get( jsInfoId_ ) );
    jsContainer.InvokeOnNotify( name_, jsValue );

    return std::nullopt;
}

} // namespace smp
