#pragma once

#include <panel/event.h>

namespace mozjs
{

class JsContainer;

}

namespace smp::panel
{

class IEvent_JsCallback
{
public:
    virtual ~IEvent_JsCallback() = default;
    virtual void InvokeJsCallback( mozjs::JsContainer& jsContainer ) = 0;
};

} // namespace smp::panel
