#include <stdafx.h>
#include "drop_utils.h"

#include <com_objects/drag_image.h>

namespace
{

template <typename T>
HRESULT GetDataObjectDataSimple( IDataObject* pDataObj, CLIPFORMAT cf, T& p_out )
{
    FORMATETC fe = { 0 };
    fe.cfFormat = cf;
    fe.dwAspect = DVASPECT_CONTENT;
    fe.lindex = -1;
    fe.tymed = TYMED_HGLOBAL;

    STGMEDIUM stgm = { 0 };

    HRESULT hr;
    if ( hr = pDataObj->GetData( &fe, &stgm ); SUCCEEDED( hr ) )
    {
        void* pData = GlobalLock( stgm.hGlobal );
        if ( pData )
        {
            p_out = *static_cast<T*>( pData );
            GlobalUnlock( pData );
        }
        ReleaseStgMedium( &stgm );
    }
    return hr;
}

HRESULT SetDataBlob( IDataObject* pdtobj, CLIPFORMAT cf, const void* pvBlob, UINT cbBlob )
{
    HRESULT hr = E_OUTOFMEMORY;
    void* pv = GlobalAlloc( GPTR, cbBlob );
    if ( !pv )
    {
        return E_OUTOFMEMORY;
    }

    CopyMemory( pv, pvBlob, cbBlob );

    FORMATETC fmte = { cf, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

    // The STGMEDIUM structure is used to define how to handle a global memory transfer.
    // This structure includes a flag, tymed, which indicates the medium
    // to be used, and a union comprising pointers and a handle for getting whichever
    // medium is specified in tymed.
    STGMEDIUM medium = { 0 };
    medium.tymed = TYMED_HGLOBAL;
    medium.hGlobal = pv;

    hr = pdtobj->SetData( &fmte, &medium, TRUE );
    if ( FAILED( hr ) )
    {
        GlobalFree( pv );
    }

    return hr;
}

pfc::string8 FormatDragText( t_size selectionCount )
{
    pfc::string8 retStr;
    retStr << selectionCount << " item";
    if ( selectionCount > 1 )
    {
        retStr << "s";
    }
    return retStr;
}

} // namespace

HRESULT SetDefaultImage( IDataObject* pdtobj )
{
    static const CLIPFORMAT cfRet = (CLIPFORMAT)RegisterClipboardFormat( L"UsingDefaultDragImage" );
    const BOOL blobValue = TRUE;
    return SetDataBlob( pdtobj, cfRet, &blobValue, sizeof( blobValue ) );
}

HRESULT SetDropText( IDataObject* pdtobj, DROPIMAGETYPE dit, const wchar_t* msg, const wchar_t* insert )
{
    static const CLIPFORMAT cfRet = (CLIPFORMAT)RegisterClipboardFormat( CFSTR_DROPDESCRIPTION );

    DROPDESCRIPTION dd_prev;
    memset( &dd_prev, 0, sizeof( dd_prev ) );

    bool dd_prev_valid = ( SUCCEEDED( GetDataObjectDataSimple( pdtobj, cfRet, dd_prev ) ) );

    // Only set the drop description if it has actually changed (otherwise things get a bit crazy near the edge of
    // the screen).
    if ( !dd_prev_valid || dd_prev.type != dit
         || wcscmp( dd_prev.szInsert, insert )
         || wcscmp( dd_prev.szMessage, msg ) )
    {
        DROPDESCRIPTION dd;
        dd.type = dit;
        wcscpy_s( dd.szMessage, msg );
        wcscpy_s( dd.szInsert, insert );
        return SetDataBlob( pdtobj, cfRet, &dd, sizeof( dd ) );
    }

    return S_OK;
}

bool RenderDragImage( HWND hWnd, size_t itemCount, const pfc::string8_fast& customDragText, SHDRAGIMAGE& dragImage )
{
    const pfc::string8 dragText = customDragText.is_empty() ? FormatDragText( itemCount ) : customDragText;
    // TODO: Add CloseThemeData(m_dd_theme) somewhere
    const HTHEME m_dd_theme = ( IsThemeActive() && IsAppThemed() ? OpenThemeData( hWnd, VSCLASS_DRAGDROP ) : nullptr );

    LOGFONT lf;
    memset( &lf, 0, sizeof( LOGFONT ) );
    SystemParametersInfo( SPI_GETICONTITLELOGFONT, 0, &lf, 0 );

    return !!uih::create_drag_image( hWnd, true, m_dd_theme, GetSysColor( COLOR_HIGHLIGHT ), GetSysColor( COLOR_HIGHLIGHTTEXT ), nullptr, &lf, !dragText.is_empty(), dragText, &dragImage );
}

HRESULT GetDragWindow( IDataObject* pDataObj, HWND& p_wnd )
{
    static const CLIPFORMAT cfRet = (CLIPFORMAT)RegisterClipboardFormat( L"DragWindow" );
    HRESULT hr;
    DWORD dw;
    if ( hr = GetDataObjectDataSimple( pDataObj, cfRet, dw ); SUCCEEDED( hr ) )
    {
        p_wnd = (HWND)ULongToHandle( dw );
    }

    return hr;
}

HRESULT GetIsShowingLayered( IDataObject* pDataObj, BOOL& p_out )
{
    static const CLIPFORMAT cfRet = (CLIPFORMAT)RegisterClipboardFormat( L"IsShowingLayered" );
    return GetDataObjectDataSimple( pDataObj, cfRet, p_out );
}
