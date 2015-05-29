#pragma once

#include "IDropTargetImpl.h"
#include "script_interface_impl.h"


class wsh_panel_window;

class HostDropTarget : public IDropTargetImpl
{
private:
	DWORD m_effect;
	wsh_panel_window *m_host;
	DropSourceAction *m_action;

	BEGIN_COM_QI_IMPL()
		COM_QI_ENTRY_MULTI(IUnknown, IDropTarget)
		COM_QI_ENTRY(IDropTarget)
	END_COM_QI_IMPL()

protected:
	virtual void FinalRelease() {}

public:
	HostDropTarget(wsh_panel_window * host);
	virtual ~HostDropTarget();

public:
	// IDropTarget
	HRESULT OnDragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
	HRESULT OnDragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
	HRESULT OnDrop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
	HRESULT OnDragLeave();

public:
	void on_drag_enter(unsigned keyState, POINTL & pt, IDropSourceAction * action);
	void on_drag_over(unsigned keyState, POINTL & pt, IDropSourceAction * action);
	void on_drag_leave();
	void on_drag_drop(unsigned keyState, POINTL & pt, IDropSourceAction * action);
};

class HostDropTargetV2 : public IDropTargetImpl
{
private:
	BEGIN_COM_QI_IMPL()
		COM_QI_ENTRY_MULTI(IUnknown, IDropTarget)
		COM_QI_ENTRY(IDropTarget)
	END_COM_QI_IMPL()

public:
	// IDropTarget
	HRESULT OnDragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
	HRESULT OnDragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
	HRESULT OnDrop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
	HRESULT OnDragLeave();

public:
	void on_drag_enter(unsigned keyState, POINTL & pt, IDropSourceAction * action);
	void on_drag_over(unsigned keyState, POINTL & pt, IDropSourceAction * action);
	void on_drag_leave();
	void on_drag_drop(unsigned keyState, POINTL & pt, IDropSourceAction * action);
};
