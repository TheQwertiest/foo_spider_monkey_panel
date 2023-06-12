#pragma once

#include <panel/panel_fwd.h>

#include <optional>

namespace smp::panel
{

class MouseMessageHandler
{
public:
    MouseMessageHandler( PanelWindow& parent );

public:
    std::optional<LRESULT> HandleMessage( const MSG& msg );
    void OnFocusMessage();

    void OnContextMenuStart();
    void OnContextMenuEnd();

    // TODO: make private
    void SetCaptureMouseState( bool shouldCapture );

private:
    PanelWindow& parent_;

    bool isMouseTracked_ = false;
    bool isMouseCaptured_ = false;
    bool isInContextMenu_ = false;
    uint8_t lastMouseDownCount_ = 0;
};

} // namespace smp::panel
