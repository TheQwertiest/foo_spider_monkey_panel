#pragma once

#include <events/event.h>

namespace smp
{

class Event_Basic : public EventBase
{
public:
    Event_Basic( EventId id );

    void Run() override;
};

} // namespace smp
