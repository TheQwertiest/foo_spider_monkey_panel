#pragma once

#include "drop_target_impl.h"
#include "script_interface_impl.h"

#include <js_objects/internal/global_heap_manager.h>

namespace mozjs
{

class JsContainer;
class JsDropSourceAction;
class JsGlobalObject;

}

class HostDropTarget
    : public IDropTargetImpl
    , public mozjs::IHeapUser
{
protected:
    virtual void FinalRelease();

public:
	HostDropTarget( JSContext * cx, HWND hWnd, mozjs::JsContainer* pJsContainer);
	virtual ~HostDropTarget();

    // IHeapUser
    virtual void DisableHeapCleanup() override;

	// IDropTarget
	HRESULT OnDragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
	HRESULT OnDragLeave();
	HRESULT OnDragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
	HRESULT OnDrop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);

private:
	void on_drag_enter(unsigned keyState, POINTL& pt);
	void on_drag_leave();
	void on_drag_over(unsigned keyState, POINTL& pt);
	void on_drag_drop(unsigned keyState, POINTL& pt);

private:
    JSContext * pJsCtx_ = nullptr;
    uint32_t objectId_;
    uint32_t globalId_;
    mozjs::JsGlobalObject* pNativeGlobal_ = nullptr;
    mozjs::JsDropSourceAction* pNativeAction_ = nullptr;

    std::mutex cleanupLock_;
    bool needsCleanup_ = false;
    mozjs::JsContainer* pJsContainer_;

	DWORD m_fb2kAllowedEffect;

	BEGIN_COM_QI_IMPL()
		COM_QI_ENTRY_MULTI(IUnknown, IDropTarget)
		COM_QI_ENTRY(IDropTarget)
	END_COM_QI_IMPL()
};
