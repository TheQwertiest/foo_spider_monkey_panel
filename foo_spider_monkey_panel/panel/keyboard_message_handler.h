#pragma once

#include <optional>

namespace smp::panel
{

class PanelWindow;

class KeyboardMessageHandler
{
public:
    KeyboardMessageHandler( PanelWindow& parent );

public:
    std::optional<LRESULT> HandleMessage( const MSG& msg );

private:
    PanelWindow& parent_;
};

} // namespace smp::panel
