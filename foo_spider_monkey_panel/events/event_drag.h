#pragma once

#include <events/event_js_executor.h>
#include <events/event_mouse.h>
#include <panel/drag_action_params.h>

namespace mozjs
{

class JsContainer;

} // namespace mozjs

namespace smp
{

class js_panel_window;

class Event_Drag
    : public Event_Mouse
{
public:
    Event_Drag( EventId id, int32_t x, int32_t y, uint32_t mask, uint32_t modifiers, const panel::DragActionParams& dragParams, IDataObjectPtr pData );

    [[nodiscard]] Event_Drag* AsDragEvent() override;

    std::optional<bool> JsExecute( mozjs::JsContainer& jsContainer ) override;

    [[nodiscard]] const panel::DragActionParams& GetDragParams() const;
    [[nodiscard]] IDataObjectPtr GetStoredData() const;

private:
    const panel::DragActionParams dragParams_;
    IDataObjectPtr pDataObject_;
};

} // namespace smp
