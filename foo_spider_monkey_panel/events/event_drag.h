#pragma once

#include <events/event_js_executor.h>
#include <events/event_mouse.h>
#include <panel/drag_action_params.h>

namespace mozjs
{

class JsContainer;

} // namespace mozjs

namespace smp::com
{

struct StorageObject;

}

namespace smp
{

class js_panel_window;

class Event_Drag
    : public Event_Mouse
{
public:
    /// @remark Should be called only from the main thread
    Event_Drag( EventId id, int32_t x, int32_t y, uint32_t mask, uint32_t modifiers, const panel::DragActionParams& dragParams, IDataObjectPtr pData );
    ~Event_Drag() override;

    [[nodiscard]] Event_Drag* AsDragEvent() override;

    std::optional<bool> JsExecute( mozjs::JsContainer& jsContainer ) override;

    [[nodiscard]] const panel::DragActionParams& GetDragParams() const;
    [[nodiscard]] IDataObjectPtr GetStoredData() const;

    /// @remark Should be called only from the main thread
    void DisposeStoredData();

private:
    const panel::DragActionParams dragParams_;
    IDataObjectPtr pDataObject_;
    com::StorageObject* pStorage_;
};

} // namespace smp
