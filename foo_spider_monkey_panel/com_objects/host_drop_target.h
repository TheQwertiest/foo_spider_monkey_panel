#pragma once

#include "drop_target_impl.h"
#include "script_interface_impl.h"

#include <drop_action_params.h>


class HostDropTarget
    : public IDropTargetImpl
{
protected:
    virtual void FinalRelease();

public:
	HostDropTarget( HWND hWnd );
	virtual ~HostDropTarget() = default;

	// IDropTarget
	HRESULT OnDragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;
	HRESULT OnDragLeave() override;
	HRESULT OnDragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;
	HRESULT OnDrop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;

private:
    void SendDragMessage( DWORD msgId, DWORD grfKeyState, POINTL pt );

private:
    mozjs::DropActionParams actionParams_;
	DWORD m_fb2kAllowedEffect = DROPEFFECT_NONE;

	BEGIN_COM_QI_IMPL()
		COM_QI_ENTRY_MULTI(IUnknown, IDropTarget)
		COM_QI_ENTRY(IDropTarget)
	END_COM_QI_IMPL()
};
