#pragma once

#include <events/event.h>

namespace smp
{

class PanelEvent : public EventBase
{
public:
    [[nodiscard]] PanelEvent( EventId id );

    [[nodiscard]] std::unique_ptr<EventBase> Clone() override;

    void Run() final;
};

} // namespace smp
