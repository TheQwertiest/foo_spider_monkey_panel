#include <stdafx.h>
#include "host_drop_target.h"
#include "js_panel_window.h"

#include <js_engine/js_container.h>
#include <js_objects/global_object.h>
#include <js_objects/drop_source_action.h>
#include <com_objects/internal/drag_utils.h>
#include <utils/location_processor.h>

#include <user_message.h>


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

HostDropTarget::HostDropTarget( HWND hWnd )
    : IDropTargetImpl( hWnd )
{
}

void HostDropTarget::FinalRelease()
{
}

HRESULT HostDropTarget::OnDragEnter( IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect )
{
    if ( !pdwEffect )
    {
        return E_POINTER;
    }

    pDataObject_ = pDataObj;
    actionParams_.Reset();

    bool native;
    HRESULT hr = ole_interaction::get()->check_dataobject( pDataObj, m_fb2kAllowedEffect, native );
    if ( FAILED( hr ) )
    {
        m_fb2kAllowedEffect = DROPEFFECT_NONE;
    }
    else if ( native && ( DROPEFFECT_MOVE & *pdwEffect ) )
    {
        m_fb2kAllowedEffect |= DROPEFFECT_MOVE; // Remove check_dataobject move suppression for intra fb2k interactions
    }

    actionParams_.effect = *pdwEffect & m_fb2kAllowedEffect;

    ScreenToClient( m_hWnd, reinterpret_cast<LPPOINT>( &pt ) );
    SendDragMessage( static_cast<UINT>( InternalSyncMessage::wnd_drag_enter ), grfKeyState, pt );

    *pdwEffect = actionParams_.effect;

    const wchar_t* dragText = ( actionParams_.text.empty() ? GetDropTextFromEffect( actionParams_.effect ) : actionParams_.text.c_str() );
    drag::SetDropText( pDataObject_, GetDropImageFromEffect( actionParams_.effect ), dragText, L"" );

    return S_OK;
}

HRESULT HostDropTarget::OnDragLeave()
{
    SendMessage( m_hWnd, static_cast<UINT>( InternalSyncMessage::wnd_drag_leave ), 0, 0 );
    drag::SetDropText( pDataObject_, DROPIMAGE_INVALID, L"", L"" );
    pDataObject_.Release();
    return S_OK;
}

HRESULT HostDropTarget::OnDragOver( DWORD grfKeyState, POINTL pt, DWORD* pdwEffect )
{
    if ( !pdwEffect )
    {
        return E_POINTER;
    }

    actionParams_.effect = *pdwEffect & m_fb2kAllowedEffect;

    ScreenToClient( m_hWnd, reinterpret_cast<LPPOINT>( &pt ) );
    SendDragMessage( static_cast<UINT>( InternalSyncMessage::wnd_drag_over ), grfKeyState, pt );

    *pdwEffect = actionParams_.effect;

    const wchar_t* dragText = ( actionParams_.text.empty() ? GetDropTextFromEffect( actionParams_.effect ) : actionParams_.text.c_str() );
    drag::SetDropText( pDataObject_, GetDropImageFromEffect( actionParams_.effect ), dragText, L"" );

    return S_OK;
}

HRESULT HostDropTarget::OnDrop( IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect )
{
    if ( !pdwEffect )
    {
        return E_POINTER;
    }

    actionParams_.effect = *pdwEffect & m_fb2kAllowedEffect;

    ScreenToClient( m_hWnd, reinterpret_cast<LPPOINT>( &pt ) );
    SendDragMessage( static_cast<UINT>( InternalSyncMessage::wnd_drag_drop ), grfKeyState, pt );

    if ( *pdwEffect == DROPEFFECT_NONE || actionParams_.effect == DROPEFFECT_NONE )
    {
        *pdwEffect = DROPEFFECT_NONE;
        return S_OK;
    }

    dropped_files_data_impl droppedData;
    HRESULT hr = ole_interaction::get()->parse_dataobject( pDataObj, droppedData );
    if ( SUCCEEDED( hr ) )
    {
        droppedData.to_handles_async_ex( playlist_incoming_item_filter_v2::op_flag_delay_ui,
                                         core_api::get_main_window(),
                                         fb2k::service_new<smp::utils::js_process_locations>( actionParams_.playlistIdx, actionParams_.base, actionParams_.toSelect ) );
    }

    *pdwEffect = actionParams_.effect;
    drag::SetDropText( pDataObject_, DROPIMAGE_INVALID, L"", L"" );
    pDataObject_.Release();

    return S_OK;
}

void HostDropTarget::SendDragMessage( DWORD msgId, DWORD grfKeyState, POINTL pt )
{
    panel::DropActionMessageParams msgParams;
    msgParams.actionParams = actionParams_;
    msgParams.keyState = grfKeyState;
    msgParams.pt = pt;
    SendMessage( m_hWnd, msgId, 0, (LPARAM)&msgParams );
    actionParams_ = msgParams.actionParams;
}

} // namespace smp::com
