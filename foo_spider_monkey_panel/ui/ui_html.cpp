#include <stdafx.h>
#include "ui_html.h"

#include <js_utils/scope_helper.h>
#include <com_objects/scriptable_web_browser.h>
#include <com_objects/dispatch_ptr.h>

#pragma warning( push )
#pragma warning( disable : 4192 )
#pragma warning( disable : 4146 )
#pragma warning( disable : 4278 )
#import <mshtml.tlb>
#pragma warning( pop )

#include <atltypes.h>

namespace smp::ui
{
using namespace mozjs;

 CDialogHtml::CDialogHtml( const std::wstring& htmlCodeOrPath )
    : htmlCodeOrPath_( htmlCodeOrPath )
{
}

LRESULT CDialogHtml::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    CAxWindow wndIE = GetDlgItem( IDC_IE );
    IObjectWithSite* pOWS = NULL;
    HRESULT hr = wndIE.QueryHost( IID_IObjectWithSite, (void**)&pOWS );
    // TODO: IF_FAILED

    hr = pOWS->SetSite( (IServiceProvider*)this );    
    // TODO: IF_FAILED

    try
    {
        IWebBrowserPtr pBrowser;

        HRESULT hr = wndIE.QueryControl( &pBrowser );
        // TODO: IF_FAILED

        pBrowser_ = pBrowser;

        _variant_t v;
        hr = pBrowser->Navigate( _bstr_t( "about:blank" ), &v, &v, &v, &v );
        // TODO: IF_FAILED

        IDispatchPtr pDocDispatch;
        hr = pBrowser->get_Document( &pDocDispatch );
        MSHTML::IHTMLDocument2Ptr pDocument = pDocDispatch;

        { // open document
            SAFEARRAY* pSaStrings = SafeArrayCreateVector( VT_VARIANT, 0, 1 );
            scope::final_action autoPsa( [pSaStrings]() {
                SafeArrayDestroy( pSaStrings );
            } );
            VARIANT* pSaVar = nullptr;
            hr = SafeArrayAccessData( pSaStrings, (LPVOID*)&pSaVar );
            //IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "SafeArrayAccessData" );
            _bstr_t bstr( htmlCodeOrPath_.c_str() );
            pSaVar->vt = VT_BSTR;
            pSaVar->bstrVal = bstr.Detach();
            hr = SafeArrayUnaccessData( pSaStrings );
            //IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "SafeArrayUnaccessData" );
            hr = pDocument->write( pSaStrings );
            //IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "write" );
        }
        hr = pDocument->close();
        //IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "close" );
        //hr = pHtaWindow->focus();
        //IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "focus" );
    }
    catch ( const _com_error& e )
    {
        pfc::string8_fast errorMsg8 = pfc::stringcvt::string_utf8_from_wide( (const wchar_t*)e.ErrorMessage() );
        pfc::string8_fast errorSource8 = pfc::stringcvt::string_utf8_from_wide( e.Source().length() ? (const wchar_t*)e.Source() : L"" );
        pfc::string8_fast errorDesc8 = pfc::stringcvt::string_utf8_from_wide( e.Description().length() ? (const wchar_t*)e.Description() : L"" );
        //JS_ReportErrorUTF8( cx, "COM error: message %s; source: %s; description: %s", errorMsg8.c_str(), errorSource8.c_str(), errorDesc8.c_str() );
    }

    return TRUE; // set focus to default control
}

LRESULT CDialogHtml::OnSize( UINT nType, CSize size )
{
    switch ( nType )
    {
    case SIZE_MAXIMIZED:
    case SIZE_RESTORED:
    {
        CAxWindow wndIE = GetDlgItem( IDC_IE );
        wndIE.ResizeClient( size.cx, size.cy );
        break;
    }
    default:
        break;
    }

    return 0;
}

LRESULT CDialogHtml::OnCloseCmd( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{    
    EndDialog( wID );
    return 0;
}

void CDialogHtml::OnTitleChange( BSTR title )
{
    try
    {
        SetWindowText( static_cast<wchar_t*>( static_cast<_bstr_t>( title ) ) );
    }
    catch ( const _com_error& )
    {
    }
}

STDMETHODIMP CDialogHtml::moveTo( LONG x, LONG y )
{
    return S_OK;
}

STDMETHODIMP CDialogHtml::moveBy( LONG x, LONG y )
{
    return S_OK;
}

STDMETHODIMP CDialogHtml::resizeTo( LONG x, LONG y )
{
    ResizeClient( x, y );
    return S_OK;
}

STDMETHODIMP CDialogHtml::resizeBy( LONG x, LONG y )
{
    if ( RECT rect; GetClientRect( &rect ) )
    {
        ResizeClient( rect.left + x, rect.top + y );
    }
    
    return S_OK;
}

ULONG STDMETHODCALLTYPE CDialogHtml::AddRef( void )
{
    return 0;
}

ULONG STDMETHODCALLTYPE CDialogHtml::Release( void )
{
    return 0;
}

} // namespace smp::ui
