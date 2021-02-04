
#include <stdafx.h>

#include "ui_file_drop_list_box.h"

namespace smp::ui
{

CFileDropListBox::CFileDropListBox( HWND hWnd )
    : smp::com::IDropTargetImpl( hWnd )
    , CListBox( hWnd )
{
}

CFileDropListBox& CFileDropListBox::operator=( HWND hWnd )
{
    CListBox::operator=( hWnd );
    return *this;
}

UINT CFileDropListBox::GetOnDropMsg()
{
    static const UINT msgId = ::RegisterWindowMessage( L"smp_ui_file_drop" );
    return msgId;
}

void CFileDropListBox::FinalRelease()
{
}

HRESULT CFileDropListBox::OnDragEnter( IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect )
{
    isFile_ = IsFile( pDataObj );
    *pdwEffect = GetEffect();

    return S_OK;
}

HRESULT CFileDropListBox::OnDragOver( DWORD grfKeyState, POINTL pt, DWORD* pdwEffect )
{
    *pdwEffect = GetEffect();
    return S_OK;
}

HRESULT CFileDropListBox::OnDrop( IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect )
{
    isFile_ = IsFile( pDataObj );
    *pdwEffect = GetEffect();

    pDataObj->AddRef();
    ::PostMessage( this->GetParent(), GetOnDropMsg(), 0, reinterpret_cast<LPARAM>( pDataObj ) );

    return S_OK;
}

HRESULT CFileDropListBox::OnDragLeave()
{
    return S_OK;
}

DWORD CFileDropListBox::GetEffect() const
{
    return ( isFile_ ? DROPEFFECT_COPY : DROPEFFECT_NONE );
}

bool CFileDropListBox::IsFile( IDataObject* pDataObj )
{
    FORMATETC fmte = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    const auto hr = pDataObj->QueryGetData( &fmte );
    return ( hr == S_OK );
}

} // namespace smp::ui
