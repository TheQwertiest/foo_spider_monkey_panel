#include "stdafx.h"
#include "helpers.h"
#include "host.h"
#include "host_droptarget.h"
#include "js_panel_window.h"

HostDropTarget::HostDropTarget(js_panel_window* host)
	: IDropTargetImpl(host->GetHWND())
	, m_host(host)
	, m_effect(DROPEFFECT_NONE)
	, m_action(new com_object_impl_t<DropSourceAction, true>())
{
}

HostDropTarget::~HostDropTarget()
{
	m_action->Release();
}

HRESULT HostDropTarget::OnDragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
	if (!pdwEffect) return E_POINTER;

	m_action->Reset();
	// Default state of parsable
	m_action->Parsable() = static_api_ptr_t<playlist_incoming_item_filter>()->process_dropped_files_check_ex(pDataObj, &m_effect);

	ScreenToClient(m_hWnd, reinterpret_cast<LPPOINT>(&pt));
	on_drag_enter(grfKeyState, pt, m_action);

	if (!m_action->Parsable())
		*pdwEffect = DROPEFFECT_NONE;
	else
		*pdwEffect = m_effect;
	return S_OK;
}

HRESULT HostDropTarget::OnDragLeave()
{
	on_drag_leave();
	return S_OK;
}

HRESULT HostDropTarget::OnDragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
	if (!pdwEffect) return E_POINTER;

	ScreenToClient(m_hWnd, reinterpret_cast<LPPOINT>(&pt));
	on_drag_over(grfKeyState, pt, m_action);

	if (!m_action->Parsable())
		*pdwEffect = DROPEFFECT_NONE;
	else
		*pdwEffect = m_effect;

	return S_OK;
}

HRESULT HostDropTarget::OnDrop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
	if (!pdwEffect) return E_POINTER;

	ScreenToClient(m_hWnd, reinterpret_cast<LPPOINT>(&pt));
	on_drag_drop(grfKeyState, pt, m_action);

	int playlist = m_action->Playlist();
	bool to_select = m_action->ToSelect();

	if (m_action->Parsable())
	{
		switch (m_action->Mode())
		{
		case DropSourceAction::kActionModePlaylist:
			static_api_ptr_t<playlist_incoming_item_filter_v2>()->process_dropped_files_async(
				pDataObj,
				playlist_incoming_item_filter_v2::op_flag_delay_ui,
				core_api::get_main_window(),
				new service_impl_t<helpers::js_process_locations>(playlist, to_select));
			break;

		case DropSourceAction::kActionModeFilenames:
			break;

		default:
			break;
		}
	}

	if (!m_action->Parsable())
		*pdwEffect = DROPEFFECT_NONE;
	else
		*pdwEffect = m_effect;

	return S_OK;
}

void HostDropTarget::on_drag_enter(unsigned keyState, POINTL& pt, IDropSourceAction* action)
{
	TRACK_FUNCTION();

	VARIANTARG args[4];
	args[0].vt = VT_I4;
	args[0].lVal = keyState;
	args[1].vt = VT_I4;
	args[1].lVal = pt.y;
	args[2].vt = VT_I4;
	args[2].lVal = pt.x;
	args[3].vt = VT_DISPATCH;
	args[3].pdispVal = action;
	m_host->script_invoke_v(CallbackIds::on_drag_enter, args, _countof(args));
}

void HostDropTarget::on_drag_leave()
{
	TRACK_FUNCTION();

	m_host->script_invoke_v(CallbackIds::on_drag_leave);
}

void HostDropTarget::on_drag_over(unsigned keyState, POINTL& pt, IDropSourceAction* action)
{
	TRACK_FUNCTION();

	VARIANTARG args[4];
	args[0].vt = VT_I4;
	args[0].lVal = keyState;
	args[1].vt = VT_I4;
	args[1].lVal = pt.y;
	args[2].vt = VT_I4;
	args[2].lVal = pt.x;
	args[3].vt = VT_DISPATCH;
	args[3].pdispVal = action;
	m_host->script_invoke_v(CallbackIds::on_drag_over, args, _countof(args));
}

void HostDropTarget::on_drag_drop(unsigned keyState, POINTL& pt, IDropSourceAction* action)
{
	TRACK_FUNCTION();

	VARIANTARG args[4];
	args[0].vt = VT_I4;
	args[0].lVal = keyState;
	args[1].vt = VT_I4;
	args[1].lVal = pt.y;
	args[2].vt = VT_I4;
	args[2].lVal = pt.x;
	args[3].vt = VT_DISPATCH;
	args[3].pdispVal = action;
	m_host->script_invoke_v(CallbackIds::on_drag_drop, args, _countof(args));
}
