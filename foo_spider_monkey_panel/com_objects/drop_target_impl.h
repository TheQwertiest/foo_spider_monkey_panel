#pragma once

#include <ShlObj.h>

_COM_SMARTPTR_TYPEDEF( IDropTargetHelper, IID_IDropTargetHelper );

namespace smp::com
{

class IDropTargetImpl : public IDropTarget
{
public:
    IDropTargetImpl( HWND hWnd ) noexcept( false );

    virtual ~IDropTargetImpl();

    HRESULT RegisterDragDrop();
    HRESULT RevokeDragDrop();

    // IDropTarget
    STDMETHODIMP DragEnter( IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect ) override;
    STDMETHODIMP DragOver( DWORD grfKeyState, POINTL pt, DWORD* pdwEffect ) override;
    STDMETHODIMP DragLeave() override;
    STDMETHODIMP Drop( IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect ) override;

    // Overrides
    virtual HRESULT OnDragEnter( IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect ) = 0;
    virtual HRESULT OnDragOver( DWORD grfKeyState, POINTL pt, DWORD* pdwEffect ) = 0;
    virtual HRESULT OnDrop( IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect ) = 0;
    virtual HRESULT OnDragLeave() = 0;

protected:
    IDropTargetHelperPtr m_dropTargetHelper;
    HWND m_hWnd;
};

} // namespace smp::com
