#include <stdafx.h>
#include "drop_target_impl.h"

#include <utils/winapi_error_helpers.h>

namespace smp::com
{

IDropTargetImpl::IDropTargetImpl( HWND hWnd )
    : m_hWnd( hWnd )
{
    assert( hWnd );

    HRESULT hr = m_dropTargetHelper.CreateInstance( CLSID_DragDropHelper, nullptr, CLSCTX_INPROC_SERVER );
    smp::error::CheckHR( hr, "CreateInstance" );

    assert( m_dropTargetHelper );
}

IDropTargetImpl::~IDropTargetImpl()
{
    RevokeDragDrop();
}

HRESULT IDropTargetImpl::RegisterDragDrop()
{
    return ::RegisterDragDrop( m_hWnd, this );
}

HRESULT IDropTargetImpl::RevokeDragDrop()
{
    return ::RevokeDragDrop( m_hWnd );
}

STDMETHODIMP IDropTargetImpl::DragEnter( IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect )
{
    if ( !pDataObj )
        return E_FAIL;
    if ( !pdwEffect )
        return E_POINTER;

    POINT point{ pt.x, pt.y };
    m_dropTargetHelper->DragEnter( m_hWnd, pDataObj, &point, *pdwEffect );

    return OnDragEnter( pDataObj, grfKeyState, pt, pdwEffect );
}

STDMETHODIMP IDropTargetImpl::DragOver( DWORD grfKeyState, POINTL pt, DWORD* pdwEffect )
{
    if ( !pdwEffect )
        return E_POINTER;

    POINT point{ pt.x, pt.y };
    m_dropTargetHelper->DragOver( &point, *pdwEffect );

    return OnDragOver( grfKeyState, pt, pdwEffect );
}

STDMETHODIMP IDropTargetImpl::DragLeave()
{
    m_dropTargetHelper->DragLeave();

    return OnDragLeave();
}

STDMETHODIMP IDropTargetImpl::Drop( IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect )
{
    if ( !pDataObj )
        return E_FAIL;
    if ( !pdwEffect )
        return E_POINTER;

    POINT point{ pt.x, pt.y };
    m_dropTargetHelper->Drop( pDataObj, &point, *pdwEffect );

    return OnDrop( pDataObj, grfKeyState, pt, pdwEffect );
}

} // namespace smp::com
