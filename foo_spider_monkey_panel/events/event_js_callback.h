#pragma once

#include <events/event.h>
#include <events/event_js_executor.h>
#include <js_engine/js_container.h>
#include <panel/js_panel_window.h>

namespace smp
{

namespace impl
{

template <typename T>
constexpr bool Contains()
{
    return false;
}

template <typename T, typename A, typename... Tail>
constexpr bool Contains()
{
    return std::is_same_v<T, A> ? true : Contains<T, Tail...>();
}

} // namespace impl

template <typename... Args>
class Event_JsCallback
    : public Event_JsExecutor
{
public:
    template <typename... ArgsFwd>
    Event_JsCallback( EventId id, ArgsFwd&&... args )
        : Event_JsExecutor( id )
        , data_( std::forward<ArgsFwd>( args )... )
    {
        static_assert( !impl::Contains<metadb_handle_list, Args...>(), "Use shared_ptr instead" );
        assert( kCallbackIdToName.count( id_ ) );
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

    [[nodiscard]] std::unique_ptr<EventBase> Clone() override
    {
        if constexpr ( std::is_copy_constructible_v<std::tuple<Args...>> )
        {
            return std::apply(
                [&]( const auto&... args ) {
                    return std::make_unique<Event_JsCallback<std::decay_t<Args>...>>( id_, args... );
                },
                data_ );
        }
        else
        {
            return nullptr;
        }
    }

private:
    std::tuple<Args...> data_;
};

template <typename... Args>
auto GenerateEvent_JsCallback( EventId id, Args&&... args )
{
    return std::make_unique<Event_JsCallback<std::decay_t<Args>...>>( id, std::forward<Args>( args )... );
}

} // namespace smp
