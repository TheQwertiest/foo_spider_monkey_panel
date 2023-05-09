#pragma once

#include <events/event.h>

namespace smp
{

class PanelEvent : public EventBase
{
public:
    PanelEvent( EventId id );

    std::unique_ptr<EventBase> Clone() override;

    void Run() final;
};

} // namespace smp
