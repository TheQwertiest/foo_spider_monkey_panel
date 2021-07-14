#pragma once

#include <events/event.h>
#include <events/ievent_js_forwarder.h>
#include <js_engine/js_container.h>
#include <panel/js_panel_window.h>

namespace smp
{

template <typename... Args>
class Event_JsCallback : public Runnable
    , public IEvent_JsTask
{
public:
    template <typename... ArgsFwd>
    Event_JsCallback( EventId id, ArgsFwd&&... args )
        : id_( id )
        , data_( std::forward<ArgsFwd>( args )... )
    {
        assert( kCallbackIdToName.count( id_ ) );
    }

    void Run( panel::js_panel_window& panelWindow ) override
    {
        assert( core_api::is_main_thread() );
        panelWindow.ExecuteJsTask( id_, *this );
    }

    std::optional<bool> JsExecute( mozjs::JsContainer& jsContainer ) override
    {
        const auto callbackName = fmt::format( "on_{}", kCallbackIdToName.at( id_ ) );
        std::apply(
            [&]( auto&&... args ) {
                jsContainer.InvokeJsCallback<bool>( callbackName, std::forward<decltype( args )>( args )... );
            },
            std::move( data_ ) );

        return std::nullopt;
    }

private:
    const EventId id_;
    std::tuple<Args...> data_;
};

template <typename... Args>
auto GenerateEvent_JsCallback( EventId id, Args&&... args )
{
    return std::make_unique<Event_JsCallback<std::decay_t<Args>...>>( id, std::forward<Args>( args )... );
}

} // namespace smp
