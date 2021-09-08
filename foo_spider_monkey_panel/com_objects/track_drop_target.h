#pragma once

#include <com_objects/com_tools.h>
#include <com_objects/drop_target_impl.h>
#include <events/event.h>
#include <panel/drag_action_params.h>

namespace smp::panel
{

class js_panel_window;

}

namespace smp::com
{

class TrackDropTarget
    : public IDropTargetImpl
{
protected:
    virtual void FinalRelease();

public:
    TrackDropTarget( panel::js_panel_window& panel );
    ~TrackDropTarget() override = default;

    // IDropTargetImpl
    DWORD OnDragEnter( IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD dwEffect ) override;
    DWORD OnDragOver( DWORD grfKeyState, POINTL pt, DWORD dwEffect ) override;
    DWORD OnDrop( IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD dwEffect ) override;
    void OnDragLeave() override;

    static void ProcessDropEvent( IDataObjectPtr pDataObject, std::optional<panel::DragActionParams> dragParamsOpt );

private:
    [[nodiscard]] std::optional<panel::DragActionParams>
    PutDragEvent( EventId eventId, DWORD grfKeyState, POINTL pt, DWORD allowedEffects );

private:
    panel::js_panel_window* pPanel_ = nullptr;
    IDataObjectPtr pDataObject_;
    DWORD fb2kAllowedEffect_ = DROPEFFECT_NONE;

    BEGIN_COM_QI_IMPL()
        COM_QI_ENTRY_MULTI( IUnknown, IDropTarget )
        COM_QI_ENTRY( IDropTarget )
    END_COM_QI_IMPL()
};

} // namespace smp::com
