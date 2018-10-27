#include "stdafx.h"
#include "host_drop_target.h"
#include "js_panel_window.h"

#include <js_engine/js_container.h>
#include <js_objects/global_object.h>
#include <js_objects/drop_source_action.h>

#include <user_message.h>


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

    actionParams_.Reset();

    bool native;
    HRESULT hr = ole_interaction::get()->check_dataobject( pDataObj, m_fb2kAllowedEffect, native );
    if ( !SUCCEEDED( hr ) )
    {
        m_fb2kAllowedEffect = DROPEFFECT_NONE;
    }
    else if ( native && (DROPEFFECT_MOVE & *pdwEffect) )
    {
        m_fb2kAllowedEffect |= DROPEFFECT_MOVE; // Remove check_dataobject move suppression for intra fb2k interactions
    }

    actionParams_.effect = *pdwEffect & m_fb2kAllowedEffect;

    ScreenToClient( m_hWnd, reinterpret_cast<LPPOINT>(&pt) );
    SendDragMessage( static_cast<UINT>(smp::PlayerMessage::wnd_drag_enter), grfKeyState, pt );

    *pdwEffect = actionParams_.effect;
    return S_OK;
}

HRESULT HostDropTarget::OnDragLeave()
{
    SendMessage( m_hWnd, static_cast<UINT>(smp::PlayerMessage::wnd_drag_leave), 0, 0 );
    return S_OK;
}

HRESULT HostDropTarget::OnDragOver( DWORD grfKeyState, POINTL pt, DWORD* pdwEffect )
{
    if ( !pdwEffect )
    {
        return E_POINTER;
    }

    actionParams_.effect = *pdwEffect & m_fb2kAllowedEffect;

    ScreenToClient( m_hWnd, reinterpret_cast<LPPOINT>(&pt) );
    SendDragMessage( static_cast<UINT>(smp::PlayerMessage::wnd_drag_over), grfKeyState, pt );

    *pdwEffect = actionParams_.effect;

    return S_OK;
}

HRESULT HostDropTarget::OnDrop( IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect )
{
    if ( !pdwEffect )
    {
        return E_POINTER;
    }

    actionParams_.effect = *pdwEffect & m_fb2kAllowedEffect;

    ScreenToClient( m_hWnd, reinterpret_cast<LPPOINT>(&pt) );
    SendDragMessage( static_cast<UINT>(smp::PlayerMessage::wnd_drag_drop), grfKeyState, pt );

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
                                         fb2k::service_new<helpers::js_process_locations>( actionParams_.playlistIdx, actionParams_.base, actionParams_.toSelect ) );
    }

    *pdwEffect = actionParams_.effect;

    return S_OK;
}

void HostDropTarget::SendDragMessage( DWORD msgId, DWORD grfKeyState, POINTL pt )
{
    mozjs::DropActionMessageParams msgParams;
    msgParams.actionParams = actionParams_;
    msgParams.keyState = grfKeyState;
    msgParams.pt = pt;
    SendMessage( m_hWnd, msgId, 0, (LPARAM)&msgParams );
    actionParams_ = msgParams.actionParams;
}
