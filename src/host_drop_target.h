#pragma once

#include "drop_target_impl.h"
#include "script_interface_impl.h"

class js_panel_window;

class HostDropTarget : public IDropTargetImpl
{
protected:
	virtual void FinalRelease()
	{
	}

public:
	HostDropTarget(js_panel_window* host);
	virtual ~HostDropTarget();

	// IDropTarget
	HRESULT OnDragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
	HRESULT OnDragLeave();
	HRESULT OnDragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
	HRESULT OnDrop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);

	void on_drag_enter(unsigned keyState, POINTL& pt, IDropSourceAction* action);
	void on_drag_leave();
	void on_drag_over(unsigned keyState, POINTL& pt, IDropSourceAction* action);
	void on_drag_drop(unsigned keyState, POINTL& pt, IDropSourceAction* action);

private:
	DWORD m_effect;
	js_panel_window* m_host;
	DropSourceAction* m_action;

	BEGIN_COM_QI_IMPL()
		COM_QI_ENTRY_MULTI(IUnknown, IDropTarget)
		COM_QI_ENTRY(IDropTarget)
	END_COM_QI_IMPL()
};
