#pragma once

#include <events/event.h>

namespace mozjs
{

class JsContainer;

}

namespace smp
{

class Event_JsExecutor : public EventBase
{
public:
    Event_JsExecutor( EventId id );
    ~Event_JsExecutor() override = default;

    void Run() final;
    virtual std::optional<bool> JsExecute( mozjs::JsContainer& jsContainer ) = 0;
};

} // namespace smp
