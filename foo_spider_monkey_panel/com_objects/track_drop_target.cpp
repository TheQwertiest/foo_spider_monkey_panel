#include <stdafx.h>

#include "track_drop_target.h"

#include <com_objects/internal/drag_utils.h>
#include <events/event_dispatcher.h>
#include <events/event_drag.h>
#include <js_engine/js_container.h>
#include <js_objects/drop_source_action.h>
#include <js_objects/global_object.h>
#include <panel/js_panel_window.h>
#include <utils/location_processor.h>

namespace
{

DROPIMAGETYPE GetDropImageFromEffect( DWORD dwEffect )
{
    if ( dwEffect & DROPEFFECT_MOVE )
    {
        return DROPIMAGE_MOVE;
    }
    if ( dwEffect & DROPEFFECT_COPY )
    {
        return DROPIMAGE_COPY;
    }
    if ( dwEffect & DROPEFFECT_LINK )
    {
        return DROPIMAGE_LINK;
    }
    if ( dwEffect & DROPEFFECT_NONE )
    {
        return DROPIMAGE_NONE;
    }
    return DROPIMAGE_INVALID;
}

const wchar_t* GetDropTextFromEffect( DWORD dwEffect )
{
    if ( dwEffect & DROPEFFECT_MOVE )
    {
        return L"Move";
    }
    if ( dwEffect & DROPEFFECT_COPY )
    {
        return L"Copy";
    }
    if ( dwEffect & DROPEFFECT_LINK )
    {
        return L"Link";
    }
    return L"";
}

} // namespace

namespace smp::com
{

TrackDropTarget::TrackDropTarget( panel::js_panel_window& panel )
    : IDropTargetImpl( panel.GetHWND() )
    , pPanel_( &panel )
{
}

void TrackDropTarget::FinalRelease()
{
    pPanel_ = nullptr;
}

DWORD TrackDropTarget::OnDragEnter( IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD dwEffect )
{
    pDataObject_ = pDataObj;

    bool native;
    HRESULT hr = ole_interaction::get()->check_dataobject( pDataObj, fb2kAllowedEffect_, native );
    if ( FAILED( hr ) )
    {
        fb2kAllowedEffect_ = DROPEFFECT_NONE;
    }
    else if ( native && ( DROPEFFECT_MOVE & dwEffect ) )
    {
        fb2kAllowedEffect_ |= DROPEFFECT_MOVE; // Remove check_dataobject move suppression for intra fb2k interactions
    }

    ScreenToClient( hWnd_, reinterpret_cast<LPPOINT>( &pt ) );
    (void)PutDragEvent( EventId::kMouseDragEnter, grfKeyState, pt, dwEffect & fb2kAllowedEffect_ );

    return DROPEFFECT_NONE;
}

DWORD TrackDropTarget::OnDragOver( DWORD grfKeyState, POINTL pt, DWORD dwEffect )
{
    ScreenToClient( hWnd_, reinterpret_cast<LPPOINT>( &pt ) );
    const auto lastDragParamsOpt = PutDragEvent( EventId::kMouseDragOver, grfKeyState, pt, dwEffect & fb2kAllowedEffect_ );
    if ( !lastDragParamsOpt )
    {
        return DROPEFFECT_NONE;
    }
    const auto& lastDragParams = *lastDragParamsOpt;

    const wchar_t* dragText = ( lastDragParams.text.empty() ? GetDropTextFromEffect( lastDragParams.effect ) : lastDragParams.text.c_str() );
    drag::SetDropText( pDataObject_, GetDropImageFromEffect( lastDragParams.effect ), dragText, L"" );

    return lastDragParams.effect;
}

DWORD TrackDropTarget::OnDrop( IDataObject* /*pDataObj*/, DWORD grfKeyState, POINTL pt, DWORD dwEffect )
{
    const auto lastDragParamsOpt = pPanel_->GetLastDragParams();

    ScreenToClient( hWnd_, reinterpret_cast<LPPOINT>( &pt ) );
    (void)PutDragEvent( EventId::kMouseDragDrop, grfKeyState, pt, dwEffect & fb2kAllowedEffect_ );

    drag::SetDropText( pDataObject_, DROPIMAGE_INVALID, L"", L"" );
    pDataObject_.Release();

    if ( !lastDragParamsOpt || dwEffect == DROPEFFECT_NONE )
    {
        return DROPEFFECT_NONE;
    }

    return lastDragParamsOpt->effect;
}

void TrackDropTarget::OnDragLeave()
{
    (void)PutDragEvent( EventId::kMouseDragLeave, {}, {}, {} );
    drag::SetDropText( pDataObject_, DROPIMAGE_INVALID, L"", L"" );
    pDataObject_.Release();
}

void TrackDropTarget::ProcessDropEvent( IDataObjectPtr pDataObject, std::optional<panel::DragActionParams> dragParamsOpt )
{
    if ( !pDataObject || !dragParamsOpt || dragParamsOpt->effect == DROPEFFECT_NONE )
    {
        return;
    }

    const auto& dragParams = *dragParamsOpt;

    dropped_files_data_impl droppedData;
    HRESULT hr = ole_interaction::get()->parse_dataobject( pDataObject.GetInterfacePtr(), droppedData );
    if ( SUCCEEDED( hr ) )
    {
        droppedData.to_handles_async_ex( playlist_incoming_item_filter_v2::op_flag_delay_ui,
                                         core_api::get_main_window(),
                                         fb2k::service_new<smp::utils::OnProcessLocationsNotify_InsertHandles>( dragParams.playlistIdx, dragParams.base, dragParams.toSelect ) );
    }
}

std::optional<panel::DragActionParams>
TrackDropTarget::PutDragEvent( EventId eventId, DWORD grfKeyState, POINTL pt, DWORD allowedEffects )
{
    if ( !pPanel_ )
    {
        return std::nullopt;
    }

    static std::unordered_map<EventId, InternalSyncMessage> eventToMsg{
        { EventId::kMouseDragEnter, InternalSyncMessage::wnd_drag_enter },
        { EventId::kMouseDragLeave, InternalSyncMessage::wnd_drag_leave },
        { EventId::kMouseDragOver, InternalSyncMessage::wnd_drag_over },
        { EventId::kMouseDragDrop, InternalSyncMessage::wnd_drag_drop }
    };

    panel::DragActionParams dragParams{};
    dragParams.effect = allowedEffects;
    dragParams.isInternal = pPanel_->HasInternalDrag();

    // process system stuff first (e.g. mouse capture)
    SendMessage( pPanel_->GetHWND(), static_cast<UINT>( eventToMsg.at( eventId ) ), 0, 0 );
    EventDispatcher::Get().PutEvent( hWnd_, std::make_unique<Event_Drag>( eventId, pt.x, pt.y, grfKeyState, GetHotkeyModifierFlags(), dragParams, pDataObject_ ), EventPriority::kInput );

    if ( eventId == EventId::kMouseDragEnter )
    { // don't want to catch left-overs from the last operation
        pPanel_->ResetLastDragParams();
    }
    return pPanel_->GetLastDragParams();
}

} // namespace smp::com
