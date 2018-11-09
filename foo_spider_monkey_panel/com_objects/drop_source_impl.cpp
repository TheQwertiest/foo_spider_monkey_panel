#include <stdafx.h>
#include "drop_source_impl.h"

#include <com_objects/drag_image.h>
#include <com_objects/drop_utils.h>

_COM_SMARTPTR_TYPEDEF( IDragSourceHelper2, IID_IDragSourceHelper2 );

IDropSourceImpl::IDropSourceImpl( HWND hWnd, IDataObject* pDataObject, size_t itemCount, const pfc::string8_fast& customDragText )
    : pDataObject_( pDataObject )
{
    try
    {
        if ( HRESULT hr = pDragSourceHelper_.CreateInstance( CLSID_DragDropHelper, nullptr, CLSCTX_INPROC_SERVER );
             SUCCEEDED( hr ) )
        {
            if ( IDragSourceHelper2Ptr pDragSourceHelper2 = pDragSourceHelper_;
                 pDragSourceHelper2 )
            {
                pDragSourceHelper2->SetFlags( DSH_ALLOWDROPDESCRIPTIONTEXT );
            }

            if ( RenderDragImage( hWnd, itemCount, customDragText, dragImage_ ) )
            {
                (void)pDragSourceHelper_->InitializeFromBitmap( &dragImage_, pDataObject );
            }

            if ( IsThemeActive() && IsAppThemed() )
            {
                (void)SetDefaultImage( pDataObject );
            }
        }
    }
    catch ( const _com_error& )
    {
    }
};

IDropSourceImpl::~IDropSourceImpl()
{
    if ( dragImage_.hbmpDragImage )
    {
        DeleteObject( dragImage_.hbmpDragImage );
    }
}

// IDropSource
STDMETHODIMP IDropSourceImpl::QueryContinueDrag( BOOL fEscapePressed, DWORD grfKeyState )
{
    if ( fEscapePressed || ( grfKeyState & MK_RBUTTON ) || ( grfKeyState & MK_MBUTTON ) )
    {
        return DRAGDROP_S_CANCEL;
    }

    if ( !( grfKeyState & MK_LBUTTON ) )
    {
        return m_dwLastEffect == DROPEFFECT_NONE ? DRAGDROP_S_CANCEL : DRAGDROP_S_DROP;
    }

    return S_OK;
}

STDMETHODIMP IDropSourceImpl::GiveFeedback( DWORD dwEffect )
{
    BOOL isShowingLayered = FALSE;
    if ( IsThemeActive() )
    {
        GetIsShowingLayered( pDataObject_, isShowingLayered );
    }

    HWND wnd_drag = nullptr;
    if ( SUCCEEDED( GetDragWindow( pDataObject_, wnd_drag ) ) && wnd_drag )
    {
        PostMessage( wnd_drag, DDWM_UPDATEWINDOW, NULL, NULL );
    }

    if ( isShowingLayered )
    {
        if ( !wasShowingLayered_ )
        { // TODO: check for leak
            auto cursor = LoadCursor( nullptr, IDC_ARROW );
            SetCursor( cursor );
        }
        if ( wnd_drag )
        {
            WPARAM wp = 1;
            if ( dwEffect & DROPEFFECT_COPY )
                wp = 3;
            else if ( dwEffect & DROPEFFECT_MOVE )
                wp = 2;
            else if ( dwEffect & DROPEFFECT_LINK )
                wp = 4;

            PostMessage( wnd_drag, WM_USER + 2, wp, NULL );
        }
    }

    wasShowingLayered_ = isShowingLayered != 0;

    m_dwLastEffect = dwEffect;
    return isShowingLayered ? S_OK : DRAGDROP_S_USEDEFAULTCURSORS;
}

ULONG STDMETHODCALLTYPE IDropSourceImpl::AddRef()
{
    return InterlockedIncrement( &m_refCount );
}

ULONG STDMETHODCALLTYPE IDropSourceImpl::Release()
{
    LONG rv = InterlockedDecrement( &m_refCount );
    if ( !rv )
    {
        delete this;
    }
    return rv;
}
