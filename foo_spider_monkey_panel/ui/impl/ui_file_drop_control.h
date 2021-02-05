#pragma once

#include <com_objects/com_tools.h>
#include <com_objects/drop_target_impl.h>

namespace smp::ui
{

template <typename T>
class CFileDropControl
    : public smp::com::IDropTargetImpl
    , public T
{
public:
    CFileDropControl( HWND hWnd );
    CFileDropControl& operator=( HWND hWnd );

    static UINT GetOnDropMsg();

protected:
    void FinalRelease();

private:
    BEGIN_COM_QI_IMPL()
        COM_QI_ENTRY_MULTI( IUnknown, IDropTarget )
        COM_QI_ENTRY( IDropTarget )
    END_COM_QI_IMPL()

    // com::IDropTargetImpl
    HRESULT OnDragEnter( IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect ) override;
    HRESULT OnDragOver( DWORD grfKeyState, POINTL pt, DWORD* pdwEffect ) override;
    HRESULT OnDrop( IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect ) override;
    HRESULT OnDragLeave() override;

    DWORD GetEffect() const;

    static bool IsFile( IDataObject* pDataObj );

private:
    bool isFile_ = false;
};

template <typename T>
CFileDropControl<T>::CFileDropControl( HWND hWnd )
    : smp::com::IDropTargetImpl( hWnd )
    , T( hWnd )
{
}

template <typename T>
CFileDropControl<T>& CFileDropControl<T>::operator=( HWND hWnd )
{
    CListBox::operator=( hWnd );
    return *this;
}

template <typename T>
UINT CFileDropControl<T>::GetOnDropMsg()
{
    static const UINT msgId = ::RegisterWindowMessage( L"smp_ui_file_drop" );
    return msgId;
}

template <typename T>
void CFileDropControl<T>::FinalRelease()
{
}

template <typename T>
HRESULT CFileDropControl<T>::OnDragEnter( IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect )
{
    isFile_ = IsFile( pDataObj );
    *pdwEffect = GetEffect();

    return S_OK;
}

template <typename T>
HRESULT CFileDropControl<T>::OnDragOver( DWORD grfKeyState, POINTL pt, DWORD* pdwEffect )
{
    *pdwEffect = GetEffect();
    return S_OK;
}

template <typename T>
HRESULT CFileDropControl<T>::OnDrop( IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect )
{
    isFile_ = IsFile( pDataObj );
    *pdwEffect = GetEffect();

    pDataObj->AddRef();
    ::PostMessage( this->GetParent(), GetOnDropMsg(), 0, reinterpret_cast<LPARAM>( pDataObj ) );

    return S_OK;
}

template <typename T>
HRESULT CFileDropControl<T>::OnDragLeave()
{
    return S_OK;
}

template <typename T>
DWORD CFileDropControl<T>::GetEffect() const
{
    return ( isFile_ ? DROPEFFECT_COPY : DROPEFFECT_NONE );
}

template <typename T>
bool CFileDropControl<T>::IsFile( IDataObject* pDataObj )
{
    FORMATETC fmte = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    const auto hr = pDataObj->QueryGetData( &fmte );
    return ( hr == S_OK );
}

using CFileDropListBox = CFileDropControl<CListBox>;

} // namespace smp::ui
