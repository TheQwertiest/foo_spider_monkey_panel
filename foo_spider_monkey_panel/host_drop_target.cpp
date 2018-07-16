#include "stdafx.h"
#include "host.h"
#include "host_drop_target.h"
#include "js_panel_window.h"

#include <js_engine/js_container.h>

HostDropTarget::HostDropTarget( HWND hWnd, mozjs::JsContainer* pJsContainer )
	: IDropTargetImpl( hWnd )
	, pJsContainer_( pJsContainer )
	, m_action(new com_object_impl_t<DropSourceAction, true>())
{
}

HostDropTarget::~HostDropTarget()
{
	m_action->Release();
}

HRESULT HostDropTarget::OnDragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    if ( !pdwEffect )
    {
        return E_POINTER;
    }

	m_action->Reset();
	
    bool native;
	HRESULT hr = ole_interaction::get()->check_dataobject(pDataObj, m_fb2kAllowedEffect, native);
	if (!SUCCEEDED(hr))
	{
		m_fb2kAllowedEffect = DROPEFFECT_NONE;
	}
	else if (native && (DROPEFFECT_MOVE & *pdwEffect))
	{
		m_fb2kAllowedEffect |= DROPEFFECT_MOVE; // Remove check_dataobject move suppression for intra fb2k interactions
	}

	m_action->Effect() = *pdwEffect & m_fb2kAllowedEffect;

	ScreenToClient(m_hWnd, reinterpret_cast<LPPOINT>(&pt));
	on_drag_enter(grfKeyState, pt, m_action);

	*pdwEffect = m_action->Effect();
	return S_OK;
}

HRESULT HostDropTarget::OnDragLeave()
{
	on_drag_leave();
	return S_OK;
}

HRESULT HostDropTarget::OnDragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    if ( !pdwEffect )
    {
        return E_POINTER;
    }

	m_action->Effect() = *pdwEffect & m_fb2kAllowedEffect;

	ScreenToClient(m_hWnd, reinterpret_cast<LPPOINT>(&pt));
	on_drag_over(grfKeyState, pt, m_action);

	*pdwEffect = m_action->Effect();

	return S_OK;
}

HRESULT HostDropTarget::OnDrop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    if ( !pdwEffect )
    {
        return E_POINTER;
    }

	m_action->Effect() = *pdwEffect & m_fb2kAllowedEffect;

	ScreenToClient(m_hWnd, reinterpret_cast<LPPOINT>(&pt));
	on_drag_drop(grfKeyState, pt, m_action);

	if (*pdwEffect == DROPEFFECT_NONE || m_action->Effect() == DROPEFFECT_NONE)
	{
		*pdwEffect = DROPEFFECT_NONE;
		return S_OK;
	}

	dropped_files_data_impl droppedData;
	HRESULT hr = ole_interaction::get()->parse_dataobject(pDataObj, droppedData);
	if (SUCCEEDED(hr))
	{
		int playlist = m_action->Playlist();
		t_size base = m_action->Base();
		bool to_select = m_action->ToSelect();

		droppedData.to_handles_async_ex(playlist_incoming_item_filter_v2::op_flag_delay_ui,
			core_api::get_main_window(),
			new service_impl_t<helpers::js_process_locations>(playlist, base, to_select));
	}

	*pdwEffect = m_action->Effect();

	return S_OK;
}

void HostDropTarget::on_drag_enter(unsigned keyState, POINTL& pt, IDropSourceAction* action)
{// TODO:
    pJsContainer_->InvokeJsCallback( "on_drag_enter",
                                     /*??? action*/
                                     static_cast<int32_t>(pt.x),
                                     static_cast<int32_t>(pt.y),
                                     static_cast<uint32_t>(keyState) );
}

void HostDropTarget::on_drag_leave()
{
    pJsContainer_->InvokeJsCallback( "on_drag_leave" );
}

void HostDropTarget::on_drag_over(unsigned keyState, POINTL& pt, IDropSourceAction* action)
{// TODO:
    pJsContainer_->InvokeJsCallback( "on_drag_over",
                                     /*??? action*/
                                     static_cast<int32_t>(pt.x),
                                     static_cast<int32_t>(pt.y),
                                     static_cast<uint32_t>(keyState) );
}

void HostDropTarget::on_drag_drop(unsigned keyState, POINTL& pt, IDropSourceAction* action)
{// TODO:
    pJsContainer_->InvokeJsCallback( "on_drag_drop",
                                     /*??? action*/
                                     static_cast<int32_t>(pt.x),
                                     static_cast<int32_t>(pt.y),
                                     static_cast<uint32_t>(keyState) );
}
