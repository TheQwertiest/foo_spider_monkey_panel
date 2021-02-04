#pragma once

#include <com_objects/com_tools.h>
#include <com_objects/host_external.h>
#include <resources/resource.h>

#include <ExDispid.h>

#include <optional>

namespace smp::ui
{
class CDialogHtml
    : public CAxDialogImpl<CDialogHtml>
    , public IServiceProviderImpl<CDialogHtml>
    , public IDispEventImpl<IDC_IE, CDialogHtml>
    , public IHTMLOMWindowServices
    , public IDocHostUIHandler
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

#pragma warning( push )
#pragma warning( disable : 6388 ) // might not be '0'
    BEGIN_SERVICE_MAP( CDialogHtml )
#pragma warning( pop )
        SERVICE_ENTRY( SID_SHTMLOMWindowServices )
    END_SERVICE_MAP()

    BEGIN_MSG_MAP( CDialogHtml )
        CHAIN_MSG_MAP( CAxDialogImpl<CDialogHtml> )
        MSG_WM_INITDIALOG( OnInitDialog )
        MSG_WM_DESTROY( OnDestroyDialog )
        MSG_WM_SIZE( OnSize )
        MSG_WM_CLOSE( OnClose )
        COMMAND_RANGE_HANDLER_EX( IDOK, IDCANCEL, OnCloseCmd )
    END_MSG_MAP()

    BEGIN_SINK_MAP( CDialogHtml )
        SINK_ENTRY( IDC_IE, DISPID_BEFORENAVIGATE2, &CDialogHtml::OnBeforeNavigate2 )
        SINK_ENTRY( IDC_IE, DISPID_TITLECHANGE, &CDialogHtml::OnTitleChange )
        SINK_ENTRY( IDC_IE, DISPID_WINDOWCLOSING, &CDialogHtml::OnWindowClosing )
    END_SINK_MAP()

public:
    CDialogHtml( JSContext* cx, const std::wstring& htmlCodeOrPath, JS::HandleValue options );
    ~CDialogHtml() override;

    LRESULT OnInitDialog( HWND hwndFocus, LPARAM lParam );
    LRESULT OnDestroyDialog();
    void OnSize( UINT nType, CSize size );
    void OnClose();
    void OnCloseCmd( WORD wNotifyCode, WORD wID, HWND hWndCtl );

    void __stdcall OnBeforeNavigate2( IDispatch* pDisp, VARIANT* URL, VARIANT* Flags,
                                      VARIANT* TargetFrameName, VARIANT* PostData, VARIANT* Headers,
                                      VARIANT_BOOL* Cancel );
    void __stdcall OnTitleChange( BSTR title );
    void __stdcall OnWindowClosing( VARIANT_BOOL bIsChildWindow, VARIANT_BOOL* Cancel );

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

    STDMETHODIMP HideUI() override;

    STDMETHODIMP UpdateUI() override;

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
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;

private:
    /// @throw qwr::QwrException
    /// @throw smp::JsException
    void ParseOptions( JS::HandleValue options );
    void SetOptions();

    static void GetMsgProc( int code, WPARAM wParam, LPARAM lParam, HWND hParent, CDialogHtml* pParent );

private:
    JSContext* pJsCtx_ = nullptr;

    const std::wstring& htmlCodeOrPath_;

    // TODO: replace with unique_ptr
    HICON hIcon_ = nullptr;

    std::optional<uint32_t> width_;
    std::optional<uint32_t> height_;
    std::optional<int32_t> x_;
    std::optional<int32_t> y_;
    bool isCentered_ = true;
    bool isContextMenuEnabled_ = true;
    bool isFormSelectionEnabled_ = false;
    bool isResizable_ = false;
    bool isScrollEnabled_ = false;

    bool isClosing_ = false;

    IHostExternalPtr pExternal_;

    IDocHostUIHandlerPtr pDefaultUiHandler_;
    IOleInPlaceActiveObjectPtr pOleInPlaceHandler_;
    uint32_t hookId_ = 0;
};

} // namespace smp::ui
