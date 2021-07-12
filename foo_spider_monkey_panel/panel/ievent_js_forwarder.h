#pragma once

#include <panel/event.h>

namespace mozjs
{

class JsContainer;

}

namespace smp::panel
{

class IEvent_JsTask
{
public:
    virtual ~IEvent_JsTask() = default;
    virtual void JsExecute( mozjs::JsContainer& jsContainer ) = 0;
};

} // namespace smp::panel
