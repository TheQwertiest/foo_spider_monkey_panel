#include <stdafx.h>

#include "drop_target_impl.h"

#include <qwr/winapi_error_helpers.h>

_COM_SMARTPTR_TYPEDEF( IDropTargetHelper, IID_IDropTargetHelper );

namespace
{

/// @throw qwr::QwrException
IDropTargetHelperPtr GetDropTargetHelper()
{
    // delay helper initialization, since it's pretty expensive
    static IDropTargetHelperPtr dth = [] {
        IDropTargetHelperPtr dth;
        HRESULT hr = dth.CreateInstance( CLSID_DragDropHelper, nullptr, CLSCTX_INPROC_SERVER );
        qwr::error::CheckHR( hr, "CreateInstance" );

        assert( dth );
        return dth;
    }();

    return dth;
}

} // namespace

namespace smp::com
{

IDropTargetImpl::IDropTargetImpl( HWND hWnd )
    : hWnd_( hWnd )
{
    assert( hWnd );
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
    GetDropTargetHelper()->DragEnter( hWnd_, pDataObj, &point, *pdwEffect );

    *pdwEffect = OnDragEnter( pDataObj, grfKeyState, pt, *pdwEffect );

    return S_OK;
}

STDMETHODIMP IDropTargetImpl::DragOver( DWORD grfKeyState, POINTL pt, DWORD* pdwEffect )
{
    if ( !pdwEffect )
    {
        return E_POINTER;
    }

    POINT point{ pt.x, pt.y };
    GetDropTargetHelper()->DragOver( &point, *pdwEffect );

    *pdwEffect = OnDragOver( grfKeyState, pt, *pdwEffect );

    return S_OK;
}

STDMETHODIMP IDropTargetImpl::DragLeave()
{
    GetDropTargetHelper()->DragLeave();

    OnDragLeave();

    return S_OK;
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
    GetDropTargetHelper()->Drop( pDataObj, &point, *pdwEffect );

    *pdwEffect = OnDrop( pDataObj, grfKeyState, pt, *pdwEffect );

    return S_OK;
}

} // namespace smp::com
