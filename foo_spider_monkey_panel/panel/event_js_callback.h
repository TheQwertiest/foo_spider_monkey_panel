#pragma once

#include <js_engine/js_container.h>
#include <panel/event.h>
#include <panel/js_callback_invoker.h>
#include <panel/js_panel_window.h>

namespace smp::panel
{

template <typename... Args>
class Event_JsCallback : public Runnable
    , public IEvent_JsCallback
{
public:
    template <typename... ArgsFwd>
    Event_JsCallback( EventId id, ArgsFwd&&... args )
        : id_( id )
        , data_( std::forward<ArgsFwd>( args )... )
    {
        assert( kCallbackIdToName.count( id_ ) );
    }

    void Run( js_panel_window& panelWindow ) override
    {
        assert( core_api::is_main_thread() );
        panelWindow.ExecuteJsCallback( id_, *this );
    }

    void InvokeJsCallback( mozjs::JsContainer& jsContainer )
    {
        const auto callbackName = fmt::format( "on_{}", kCallbackIdToName.at( id_ ) );
        std::apply( [&]( auto&&... args ) { jsContainer.InvokeJsCallback( callbackName, args... ); }, data_ );
    }

private:
    const EventId id_;
    std::tuple<Args...> data_;
};

} // namespace smp::panel
