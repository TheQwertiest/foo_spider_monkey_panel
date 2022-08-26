#pragma once

#include <events/event_js_executor.h>

namespace mozjs
{

class JsContainer;

}

namespace smp
{

class JsCallbackEventNew : public Event_JsExecutor
{
public:
    JsCallbackEventNew( EventId id );
    ~JsCallbackEventNew() override = default;

    std::unique_ptr<EventBase> Clone() override;
    std::optional<bool> JsExecute( mozjs::JsContainer& jsContainer ) override;
};

} // namespace smp
