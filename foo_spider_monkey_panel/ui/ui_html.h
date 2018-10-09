#pragma once

#include "resource.h"
#include <com_objects/com_tools.h>

#include <ExDispid.h>

namespace smp::ui
{
class CDialogHtml
    : public CAxDialogImpl<CDialogHtml>
    , public IServiceProviderImpl<CDialogHtml>
    , public IHTMLOMWindowServices
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
    CDialogHtml( const std::wstring& htmlCodeOrPath );

    LRESULT OnInitDialog( HWND hwndFocus, LPARAM lParam );
    LRESULT OnSize( UINT nType, CSize size );
    LRESULT OnCloseCmd( WORD wNotifyCode, WORD wID, HWND hWndCtl );

    void __stdcall OnTitleChange( BSTR title );

    // IHTMLOMWindowServices
    STDMETHODIMP moveTo( LONG x, LONG y ) override;
    STDMETHODIMP moveBy( LONG x, LONG y ) override;
    STDMETHODIMP resizeTo( LONG x, LONG y ) override;
    STDMETHODIMP resizeBy( LONG x, LONG y ) override;

    // IUnknown
    ULONG STDMETHODCALLTYPE AddRef( void ) override;
    ULONG STDMETHODCALLTYPE Release( void ) override;

private:
    const std::wstring& htmlCodeOrPath_;
    IWebBrowserPtr pBrowser_;
};

} // namespace smp::ui
