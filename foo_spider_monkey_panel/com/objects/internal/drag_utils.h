#pragma once

#define DDWM_SETCURSOR    ( WM_USER + 2 )
#define DDWM_UPDATEWINDOW ( WM_USER + 3 )

namespace smp::com::drag
{

HRESULT SetDefaultImage( IDataObject* pdtobj );
HRESULT SetDropText( IDataObject* pdtobj, DROPIMAGETYPE dit, const wchar_t* msg, const wchar_t* insert );
bool RenderDragImage( HWND hWnd, size_t itemCount, bool isThemed, bool showText, Gdiplus::Bitmap* pCustomImage, SHDRAGIMAGE& dragImage );
HRESULT GetDragWindow( IDataObject* pDataObj, HWND& p_wnd );
HRESULT GetIsShowingLayered( IDataObject* pDataObj, BOOL& p_out );

} // namespace smp::com::drag
