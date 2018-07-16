#pragma once

#include "drop_target_impl.h"
#include "script_interface_impl.h"

namespace mozjs
{

class JsContainer;

}

class HostDropTarget : public IDropTargetImpl
{
protected:
	virtual void FinalRelease()
	{
	}

public:
	HostDropTarget( HWND hWnd, mozjs::JsContainer* pJsContainer);
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
    mozjs::JsContainer* pJsContainer_ = nullptr;

	DropSourceAction* m_action = nullptr;
	DWORD m_fb2kAllowedEffect;

	BEGIN_COM_QI_IMPL()
		COM_QI_ENTRY_MULTI(IUnknown, IDropTarget)
		COM_QI_ENTRY(IDropTarget)
	END_COM_QI_IMPL()
};
