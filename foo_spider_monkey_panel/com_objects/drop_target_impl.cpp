#include <stdafx.h>
#include "drop_target_impl.h"

#include <utils/winapi_error_helper.h>

namespace smp::com
{

IDropTargetImpl::IDropTargetImpl( HWND hWnd )
    : m_hWnd( hWnd )
{
    assert( hWnd );

    HRESULT hr = m_dropTargetHelper.CreateInstance( CLSID_DragDropHelper, nullptr, CLSCTX_INPROC_SERVER );
    smp::error::CheckHR( hr, "CreateInstance" );
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

    HRESULT hr = S_OK;
    try
    {
        if ( m_dropTargetHelper )
        {
            POINT point = { pt.x, pt.y };
            m_dropTargetHelper->DragEnter( m_hWnd, pDataObj, &point, *pdwEffect );
        }

        hr = OnDragEnter( pDataObj, grfKeyState, pt, pdwEffect );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    catch ( ... )
    {
        return E_FAIL;
    }
    return hr;
}

STDMETHODIMP IDropTargetImpl::DragOver( DWORD grfKeyState, POINTL pt, DWORD* pdwEffect )
{
    if ( !pdwEffect )
        return E_POINTER;

    HRESULT hr = S_OK;
    try
    {
        if ( m_dropTargetHelper )
        {
            POINT point = { pt.x, pt.y };
            m_dropTargetHelper->DragOver( &point, *pdwEffect );
        }

        hr = OnDragOver( grfKeyState, pt, pdwEffect );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    catch ( ... )
    {
        return E_FAIL;
    }
    return hr;
}

STDMETHODIMP IDropTargetImpl::DragLeave()
{
    HRESULT hr = S_OK;

    try
    {
        if ( m_dropTargetHelper )
        {
            m_dropTargetHelper->DragLeave();
        }

        hr = OnDragLeave();
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    catch ( ... )
    {
        return E_FAIL;
    }
    return hr;
}

STDMETHODIMP IDropTargetImpl::Drop( IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect )
{
    HRESULT hr = S_OK;

    if ( !pDataObj )
        return E_FAIL;
    if ( !pdwEffect )
        return E_POINTER;

    try
    {
        if ( m_dropTargetHelper )
        {
            POINT point = { pt.x, pt.y };
            m_dropTargetHelper->Drop( pDataObj, &point, *pdwEffect );
        }

        hr = OnDrop( pDataObj, grfKeyState, pt, pdwEffect );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    catch ( ... )
    {
        return E_FAIL;
    }
    return hr;
}

} // namespace smp::com
