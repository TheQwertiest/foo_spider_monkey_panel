
#define DDWM_UPDATEWINDOW ( WM_USER + 3 )

HRESULT SetDefaultImage( IDataObject* pdtobj );
HRESULT SetDropText( IDataObject* pdtobj, DROPIMAGETYPE dit, const wchar_t* msg, const wchar_t* insert );
bool RenderDragImage( HWND hWnd, size_t itemCount, const pfc::string8_fast& customDragText, SHDRAGIMAGE& dragImage );
HRESULT GetDragWindow( IDataObject* pDataObj, HWND& p_wnd );
HRESULT GetIsShowingLayered( IDataObject* pDataObj, BOOL& p_out );
