#include <stdafx.h>

#include "drop_target_impl.h"

#include <utils/logging.h>

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
    try
    {
        GetDropTargetHelper()->DragEnter( hWnd_, pDataObj, &point, *pdwEffect );
    }
    catch ( const qwr::QwrException& e )
    {
        smp::utils::LogWarning( fmt::format( "DnD initialization failed:\n"
                                             "  {}",
                                             e.what() ) );
        return E_FAIL;
    }

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
    try
    {
        GetDropTargetHelper()->DragOver( &point, *pdwEffect );
    }
    catch ( const qwr::QwrException& e )
    {
        smp::utils::LogWarning( fmt::format( "DnD initialization failed:\n"
                                             "  {}",
                                             e.what() ) );
        return E_FAIL;
    }

    *pdwEffect = OnDragOver( grfKeyState, pt, *pdwEffect );

    return S_OK;
}

STDMETHODIMP IDropTargetImpl::DragLeave()
{
    try
    {
        GetDropTargetHelper()->DragLeave();
    }
    catch ( const qwr::QwrException& e )
    {
        smp::utils::LogWarning( fmt::format( "DnD initialization failed:\n"
                                             "  {}",
                                             e.what() ) );
        return E_FAIL;
    }

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
    try
    {
        GetDropTargetHelper()->Drop( pDataObj, &point, *pdwEffect );
    }
    catch ( const qwr::QwrException& e )
    {
        smp::utils::LogWarning( fmt::format( "DnD initialization failed:\n"
                                             "  {}",
                                             e.what() ) );
        return E_FAIL;
    }

    *pdwEffect = OnDrop( pDataObj, grfKeyState, pt, *pdwEffect );

    return S_OK;
}

} // namespace smp::com
