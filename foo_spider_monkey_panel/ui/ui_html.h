#pragma once

#include "resource.h"
#include <com_objects/com_tools.h>
#include <com_objects/host_external.h>

#include <ExDispid.h>

namespace smp::ui
{
class CDialogHtml
    : public CAxDialogImpl<CDialogHtml>
    , public IServiceProviderImpl<CDialogHtml>
    , public IHTMLOMWindowServices
    , public IDocHostUIHandler
    , public IDispEventImpl<IDC_IE, CDialogHtml>
{
public:
    enum
    {
        IDD = IDD_DIALOG_HTML
    };

    BEGIN_COM_QI_IMPL()
    COM_QI_ENTRY_MULTI( IUnknown, IServiceProvider )
    COM_QI_ENTRY( IHTMLOMWindowServices )
    COM_QI_ENTRY( IServiceProvider )
    END_COM_QI_IMPL()

    BEGIN_SERVICE_MAP( CDialogHtml )
    SERVICE_ENTRY( SID_SHTMLOMWindowServices )
    END_SERVICE_MAP()

    BEGIN_MSG_MAP( CDialogHtml )
    CHAIN_MSG_MAP( CAxDialogImpl<CDialogHtml> )
    MSG_WM_INITDIALOG( OnInitDialog )
    MSG_WM_SIZE( OnSize )
    COMMAND_RANGE_HANDLER_EX( IDOK, IDCANCEL, OnCloseCmd )
    END_MSG_MAP()

    BEGIN_SINK_MAP( CDialogHtml )
    SINK_ENTRY( IDC_IE, DISPID_TITLECHANGE, OnTitleChange )
    END_SINK_MAP()

public:
    CDialogHtml( JSContext* cx, const std::wstring& htmlCodeOrPath, JS::HandleValue options );
    ~CDialogHtml() = default;

    LRESULT OnInitDialog( HWND hwndFocus, LPARAM lParam );
    LRESULT OnSize( UINT nType, CSize size );
    LRESULT OnCloseCmd( WORD wNotifyCode, WORD wID, HWND hWndCtl );

    void __stdcall OnTitleChange( BSTR title );

    // IHTMLOMWindowServices
    STDMETHODIMP moveTo( LONG x, LONG y ) override;
    STDMETHODIMP moveBy( LONG x, LONG y ) override;
    STDMETHODIMP resizeTo( LONG x, LONG y ) override;
    STDMETHODIMP resizeBy( LONG x, LONG y ) override;

    // IDocHostUIHandler
    STDMETHODIMP ShowContextMenu(
        DWORD dwID,
        POINT* ppt,
        IUnknown* pcmdtReserved,
        IDispatch* pdispReserved ) override;

    STDMETHODIMP GetHostInfo(
        DOCHOSTUIINFO* pInfo ) override;

    STDMETHODIMP ShowUI(
        DWORD dwID,
        IOleInPlaceActiveObject* pActiveObject,
        IOleCommandTarget* pCommandTarget,
        IOleInPlaceFrame* pFrame,
        IOleInPlaceUIWindow* pDoc ) override;

    STDMETHODIMP HideUI( void ) override;

    STDMETHODIMP UpdateUI( void ) override;

    STDMETHODIMP EnableModeless(
        BOOL fEnable ) override;

    STDMETHODIMP OnDocWindowActivate(
        BOOL fActivate ) override;

    STDMETHODIMP OnFrameWindowActivate(
        BOOL fActivate ) override;

    STDMETHODIMP ResizeBorder(
        LPCRECT prcBorder,
        IOleInPlaceUIWindow* pUIWindow,
        BOOL fRameWindow ) override;

    STDMETHODIMP TranslateAccelerator(
        LPMSG lpMsg,
        const GUID* pguidCmdGroup,
        DWORD nCmdID ) override;

    STDMETHODIMP GetOptionKeyPath(
        LPOLESTR* pchKey,
        DWORD dw ) override;

    STDMETHODIMP GetDropTarget(
        IDropTarget* pDropTarget,
        IDropTarget** ppDropTarget ) override;

    STDMETHODIMP GetExternal(
        IDispatch** ppDispatch ) override;

    STDMETHODIMP TranslateUrl(
        DWORD dwTranslate,
        LPWSTR pchURLIn,
        LPWSTR* ppchURLOut ) override;

    STDMETHODIMP FilterDataObject(
        IDataObject* pDO,
        IDataObject** ppDORet ) override;

    // IUnknown
    ULONG STDMETHODCALLTYPE AddRef( void ) override;
    ULONG STDMETHODCALLTYPE Release( void ) override;

private:
    bool ParseOptions( JS::HandleValue options );

private:
    JSContext* pJsCtx_ = nullptr;

    const std::wstring& htmlCodeOrPath_;

    // TODO: replace with exception
    bool isInitSuccess = false;

    bool showContextMenu_ = false;
    bool enableSelection_ = false;    
    IHostExternalPtr pExternal_;

    IDocHostUIHandlerPtr pDefaultUiHandler_;
};

} // namespace smp::ui
