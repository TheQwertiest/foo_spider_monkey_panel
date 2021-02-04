#include <stdafx.h>

#include "drop_target_impl.h"

#include <qwr/winapi_error_helpers.h>

namespace smp::com
{

IDropTargetImpl::IDropTargetImpl( HWND hWnd )
    : hWnd_( hWnd )
{
    assert( hWnd );

    HRESULT hr = m_dropTargetHelper.CreateInstance( CLSID_DragDropHelper, nullptr, CLSCTX_INPROC_SERVER );
    qwr::error::CheckHR( hr, "CreateInstance" );

    assert( m_dropTargetHelper );
}

IDropTargetImpl::~IDropTargetImpl()
{
    RevokeDragDrop();
}

HRESULT IDropTargetImpl::RegisterDragDrop()
{
    return ::RegisterDragDrop( hWnd_, this );
}

HRESULT IDropTargetImpl::RevokeDragDrop()
{
    return ::RevokeDragDrop( hWnd_ );
}

STDMETHODIMP IDropTargetImpl::DragEnter( IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect )
{
    if ( !pDataObj )
    {
        return E_FAIL;
    }
    if ( !pdwEffect )
    {
        return E_POINTER;
    }

    POINT point{ pt.x, pt.y };
    m_dropTargetHelper->DragEnter( hWnd_, pDataObj, &point, *pdwEffect );

    return OnDragEnter( pDataObj, grfKeyState, pt, pdwEffect );
}

STDMETHODIMP IDropTargetImpl::DragOver( DWORD grfKeyState, POINTL pt, DWORD* pdwEffect )
{
    if ( !pdwEffect )
    {
        return E_POINTER;
    }

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
    {
        return E_FAIL;
    }
    if ( !pdwEffect )
    {
        return E_POINTER;
    }

    POINT point{ pt.x, pt.y };
    m_dropTargetHelper->Drop( pDataObj, &point, *pdwEffect );

    return OnDrop( pDataObj, grfKeyState, pt, pdwEffect );
}

} // namespace smp::com
