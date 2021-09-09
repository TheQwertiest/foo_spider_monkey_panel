#include <stdafx.h>

#include "file_drop_target.h"

namespace smp::com
{

FileDropTarget::FileDropTarget( HWND hDropWnd, HWND hNotifyWnd )
    : IDropTargetImpl( hDropWnd )
    , hDropWnd_( hDropWnd )
    , hNotifyWnd_( hNotifyWnd )
{
}

UINT FileDropTarget::GetOnDropMsg()
{
    static const auto msgId = ::RegisterWindowMessage( L"smp_file_drop" );
    return msgId;
}

void FileDropTarget::FinalRelease()
{
}

DWORD FileDropTarget::OnDragEnter( IDataObject* pDataObj, DWORD /*grfKeyState*/, POINTL /*pt*/, DWORD dwEffect )
{
    isFile_ = IsFile( pDataObj );
    return GetEffect();
}

DWORD FileDropTarget::OnDragOver( DWORD /*grfKeyState*/, POINTL /*pt*/, DWORD dwEffect )
{
    return GetEffect();
}

DWORD FileDropTarget::OnDrop( IDataObject* pDataObj, DWORD /*grfKeyState*/, POINTL /*pt*/, DWORD dwEffect )
{
    isFile_ = IsFile( pDataObj );
    const auto newEffect = GetEffect();

    pDataObj->AddRef();
    if ( !PostMessage( hNotifyWnd_, GetOnDropMsg(), reinterpret_cast<WPARAM>( hDropWnd_ ), reinterpret_cast<LPARAM>( pDataObj ) ) )
    {
        pDataObj->Release();
    }

    return newEffect;
}

void FileDropTarget::OnDragLeave()
{
}

DWORD FileDropTarget::GetEffect() const
{
    return ( isFile_ ? DROPEFFECT_COPY : DROPEFFECT_NONE );
}

bool FileDropTarget::IsFile( IDataObject* pDataObj )
{
    FORMATETC fmte{ CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    const auto hr = pDataObj->QueryGetData( &fmte );
    return ( hr == S_OK );
}

} // namespace smp::com
