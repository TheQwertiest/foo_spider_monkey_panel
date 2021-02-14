#include <stdafx.h>

#include "file_drop_target.h"

namespace smp::com
{

FileDropTarget::FileDropTarget( HWND hDropWnd, HWND hNotifyWnd )
    : smp::com::IDropTargetImpl( hDropWnd )
    , hDropWnd_( hDropWnd )
    , hNotifyWnd_( hNotifyWnd )
{
}

UINT FileDropTarget::GetOnDropMsg()
{
    static const UINT msgId = ::RegisterWindowMessage( L"smp_file_drop" );
    return msgId;
}

void FileDropTarget::FinalRelease()
{
}

HRESULT FileDropTarget::OnDragEnter( IDataObject* pDataObj, DWORD /*grfKeyState*/, POINTL /*pt*/, DWORD* pdwEffect )
{
    isFile_ = IsFile( pDataObj );
    *pdwEffect = GetEffect();

    return S_OK;
}

HRESULT FileDropTarget::OnDragOver( DWORD /*grfKeyState*/, POINTL /*pt*/, DWORD* pdwEffect )
{
    *pdwEffect = GetEffect();
    return S_OK;
}

HRESULT FileDropTarget::OnDrop( IDataObject* pDataObj, DWORD /*grfKeyState*/, POINTL /*pt*/, DWORD* pdwEffect )
{
    isFile_ = IsFile( pDataObj );
    *pdwEffect = GetEffect();

    pDataObj->AddRef();
    if ( !PostMessage( hNotifyWnd_, GetOnDropMsg(), reinterpret_cast<WPARAM>( hDropWnd_ ), reinterpret_cast<LPARAM>( pDataObj ) ) )
    {
        pDataObj->Release();
    }

    return S_OK;
}

HRESULT FileDropTarget::OnDragLeave()
{
    return S_OK;
}

DWORD FileDropTarget::GetEffect() const
{
    return ( isFile_ ? DROPEFFECT_COPY : DROPEFFECT_NONE );
}

bool FileDropTarget::IsFile( IDataObject* pDataObj )
{
    FORMATETC fmte = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    const auto hr = pDataObj->QueryGetData( &fmte );
    return ( hr == S_OK );
}

} // namespace smp::com
