#pragma once

#include <com_objects/com_tools.h>

#include <ShlObj.h>

_COM_SMARTPTR_TYPEDEF( IDragSourceHelper, IID_IDragSourceHelper );

namespace smp::com

{

class IDropSourceImpl : public IDropSource
{
public:
    IDropSourceImpl( HWND hWnd, IDataObject* pDataObject, size_t itemCount, const pfc::string8_fast& customDragText ) noexcept( false );
    virtual ~IDropSourceImpl();

    // IDropSource
    STDMETHODIMP QueryContinueDrag( BOOL fEscapePressed, DWORD grfKeyState );
    STDMETHODIMP GiveFeedback( DWORD dwEffect );
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

private:
    IDragSourceHelperPtr pDragSourceHelper_;
    IDataObject* pDataObject_ = nullptr;
    SHDRAGIMAGE dragImage_ = { 0 };

    bool wasShowingLayered_ = false;

    LONG m_refCount = 0;
    DWORD m_dwLastEffect = DROPEFFECT_NONE;

    BEGIN_COM_QI_IMPL()
    COM_QI_ENTRY_MULTI( IUnknown, IDropSource )
    COM_QI_ENTRY( IDropSource )
    END_COM_QI_IMPL()
};

} // namespace smp::com
