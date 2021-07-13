#pragma once

#include <panel/event.h>

namespace mozjs
{

class JsContainer;

}

namespace smp::panel
{

class IEvent_JsTask : public EventBase
{
public:
    ~IEvent_JsTask() override = default;
    virtual std::optional<bool> JsExecute( mozjs::JsContainer& jsContainer ) = 0;
};

} // namespace smp::panel
