#include <stdafx.h>
#include "html_window.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/gdi_error_helper.h>
#include <js_utils/winapi_error_helper.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/dispatch_ptr.h>
#include <js_utils/scope_helper.h>
#include <convert/com.h>

// std::time
#include <ctime>

#pragma warning( push )
#pragma warning(disable: 4192)
#pragma warning(disable: 4146)
#pragma warning(disable: 4278)
#   import <mshtml.tlb>
#   import <shdocvw.dll>
#   undef GetWindowStyle
#   undef GetWindowLong
#   undef GetFreeSpace
#   import <wshom.ocx>
#pragma warning( pop ) 

namespace
{

using namespace mozjs;

JSClassOps jsOps = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    JsHtmlWindow::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "HtmlWindow",
    DefaultClassFlags(),
    &jsOps
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsHtmlWindow, Close )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "Close", Close , 0, DefaultPropsFlags() ),
    JS_FS_END
};

const JSPropertySpec jsProperties[] = {
    JS_PS_END
};

}

namespace mozjs
{

const JSClass JsHtmlWindow::JsClass = jsClass;
const JSFunctionSpec* JsHtmlWindow::JsFunctions = jsFunctions;
const JSPropertySpec* JsHtmlWindow::JsProperties = jsProperties;
const JsPrototypeId JsHtmlWindow::PrototypeId = JsPrototypeId::HtmlWindow;

JsHtmlWindow::JsHtmlWindow( JSContext* cx, HtmlWindow2ComPtr pHtaWindow )
    : pJsCtx_( cx )
    , pHtaWindow_( pHtaWindow )
{
}

JsHtmlWindow::~JsHtmlWindow()
{
}

std::unique_ptr<JsHtmlWindow>
JsHtmlWindow::CreateNative( JSContext* cx, const std::wstring& htmlCode, const std::wstring& data, JS::HandleValue callback )
{
    if ( !(callback.isObject() && JS_ObjectIsFunction( cx, &callback.toObject() ))
         && !callback.isNullOrUndefined() )
    {
        JS_ReportErrorUTF8( cx, "callback argument is not a function" );
        return nullptr;
    }

    JS::RootedFunction jsFunction( cx );
    if ( callback.isObject() )
    {
        jsFunction.set( JS_ValueToFunction( cx, callback ) );
    }

    try
    {
        const auto wndId = []
        {
            std::srand( unsigned( std::time( 0 ) ) );
            return L"a" + std::to_wstring( std::rand() );
        }();

        const std::wstring features =
            L"singleinstance=yes "
            L"border=dialog "
            L"minimizeButton=no "
            L"maximizeButton=no "
            L"scroll=no "
            L"showintaskbar=yes "
            L"contextMenu=yes "
            L"selection=no "
            L"innerBorder=no";
        const auto htaCode =
            L"<script>moveTo(-1000,-1000);resizeTo(0,0);</script>"
            L"<hta:application id=app " + features + L" />"
            L"<object id='" + wndId + L"' style='display:none' classid='clsid:8856F961-340A-11D0-A96B-00C04FD705A2'>"
            L"    <param name=RegisterAsBrowser value=1>"
            L"</object>";

        const std::wstring launchCmd = L"\"about:" + htaCode + L"\"";

        HINSTANCE dwRet = ShellExecute( nullptr, L"open", L"mshta.exe", launchCmd.c_str(), nullptr, 1 );
        if ( (DWORD)dwRet <= 32 )
        {// yep, WinAPI logic
            JS_ReportErrorUTF8( cx, "ShellExecute failed: %u", dwRet );
            return nullptr;
        }

        /*
        SHELLEXECUTEINFO execInfo = { 0 };
        execInfo.cbSize = sizeof( execInfo );
        execInfo.lpVerb = L"open";
        execInfo.lpFile = L"mshta.exe";
        execInfo.lpParameters = launchCmd.c_str();
        execInfo.fMask = SEE_MASK_NOCLOSEPROCESS;

        BOOL bRet = ShellExecuteEx( &execInfo );
        IF_WINAPI_FAILED_RETURN_WITH_REPORT( cx, bRet && ((DWORD)execInfo.hInstApp > 32), nullptr, ShellExecuteEx );

        struct ProcessWindowsInfo
        {
            DWORD ProcessID;
            HWND hwnd;

            ProcessWindowsInfo( DWORD const AProcessID )
                : ProcessID( AProcessID )
            {
            }
        };

        auto EnumProcessWindowsProc = []( HWND hwnd, LPARAM lParam ) -> BOOL
        {
            ProcessWindowsInfo *Info = reinterpret_cast<ProcessWindowsInfo*>(lParam);
            DWORD WindowProcessID;

            GetWindowThreadProcessId( hwnd, &WindowProcessID );

            if ( WindowProcessID == Info->ProcessID )
            {
                Info->hwnd = hwnd;
            }

            return true;
        };

        ProcessWindowsInfo Info( GetProcessId( execInfo.hProcess ) );

        bRet = EnumWindows( (WNDENUMPROC)EnumProcessWindowsProc,
                            reinterpret_cast<LPARAM>(&Info) );
        IF_WINAPI_FAILED_RETURN_WITH_REPORT( cx, bRet, nullptr, EnumWindows );
        */

        Sleep( 100 ); ///< Give some time for window to spawn

        IShellDispatchPtr pShell;
        HRESULT hr = pShell.GetActiveObject( CLSID_Shell );
        if ( FAILED( hr ) )
        {
            hr = pShell.CreateInstance( CLSID_Shell, nullptr, CLSCTX_INPROC_SERVER );
            IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "CreateInstance" );
        }

        IDispatchPtr pDispatch;
        hr = pShell->Windows( &pDispatch );
        IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "Windows" );

        SHDocVw::IShellWindowsPtr pShellWindows( pDispatch );
        MSHTML::IHTMLWindow2Ptr pHtaWindow;
        MSHTML::IHTMLDocument2Ptr pDocument;
        for ( long i = pShellWindows->GetCount() - 1; i >= 0; --i )
        {// Ideally we should use HWND instead, but GetHWND fails for no apparent reason...
            _variant_t va( i, VT_I4 );
            CDispatchPtr pCurWindow = pShellWindows->Item( va );
            _variant_t idVal = pCurWindow.Get( L"id" );
            if ( idVal.vt == VT_EMPTY )
            {
                continue;
            }
            
            if ( static_cast<_bstr_t>(idVal) != _bstr_t( wndId.c_str() ) )
            {
                continue;
            }

            pDocument = pCurWindow.Get( L"parent" );
            if ( !pDocument )
            {
                JS_ReportErrorUTF8( cx, "Failed to get IHTMLDocument2" );
                return nullptr;
            }

            pHtaWindow = pDocument->GetparentWindow();
            if ( !pHtaWindow )
            {
                JS_ReportErrorUTF8( cx, "Failed to get IHTMLWindow2" );
                return nullptr;
            }

            break;
        }
        if ( !pHtaWindow )
        {
            JS_ReportErrorUTF8( cx, "Failed to create HTML window" );
            return nullptr;
        }

        IDispatchExPtr pHtaWindowEx( pHtaWindow );
       
        {// open document
            SAFEARRAY* pSaStrings = SafeArrayCreateVector( VT_VARIANT, 0, 1 );
            scope::final_action autoPsa( [pSaStrings]()
            {
                SafeArrayDestroy( pSaStrings );
            } );

            VARIANT *pSaVar = nullptr;
            hr = SafeArrayAccessData( pSaStrings, (LPVOID*)&pSaVar );
            IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "SafeArrayAccessData" );

            _bstr_t bstr( L"" );
            pSaVar->vt = VT_BSTR;
            pSaVar->bstrVal = bstr.Detach();

            hr = SafeArrayUnaccessData( pSaStrings );
            IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "SafeArrayUnaccessData" );

            // TODO: investigate if it's possible to replace with document.open()
            hr = pDocument->write( pSaStrings );
            IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "write" );
        }

        {// store callback data
            DISPID dispId;
            _bstr_t varName = L"callback_data";
            hr = pHtaWindowEx->GetDispID( varName.GetBSTR(), fdexNameEnsure, &dispId );
            IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "GetDispID" );

            _variant_t callbackData( data.c_str() );
            DISPID dispPut = { DISPID_PROPERTYPUT };
            DISPPARAMS dispParams = { &callbackData.GetVARIANT(), &dispPut, 1, 1 };

            hr = pHtaWindowEx->InvokeEx( dispId, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT, &dispParams, nullptr, nullptr, nullptr );
            IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "InvokeEx" );
        }

        {// store callback function
            DISPID dispId;
            _bstr_t varName = L"callback_fn";
            hr = pHtaWindowEx->GetDispID( varName.GetBSTR(), fdexNameEnsure, &dispId );
            IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "GetDispID" );

            _variant_t callbackFn;
            convert::com::JsToVariant( cx, callback, callbackFn.GetVARIANT() );

            DISPID dispPut = { DISPID_PROPERTYPUT };
            DISPPARAMS dispParams = { &callbackFn.GetVARIANT(), &dispPut, 1, 1 };

            hr = pHtaWindowEx->InvokeEx( dispId, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT, &dispParams, nullptr, nullptr, nullptr );
            IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "InvokeEx" );
        }

        {// write html
            _bstr_t bstr =
                _bstr_t( htmlCode.c_str() ) +
                L"<script id=\"" + wndId.c_str() + "\">"
                L"    document.title='azaza';"
                L"    var width = 800;"
                L"    var height = 800;"
                L"    resizeTo(width, height);" +
                L"    moveTo((screen.width-width)/2, (screen.height-height)/2);"
                L"    document.getElementById('" + wndId.c_str() + "').removeNode();"
                L"</script>";

            SAFEARRAY* pSaStrings = SafeArrayCreateVector( VT_VARIANT, 0, 1 );
            scope::final_action autoPsa( [pSaStrings]()
            {
                SafeArrayDestroy( pSaStrings );
            } );

            VARIANT *pSaVar = nullptr;
            hr = SafeArrayAccessData( pSaStrings, (LPVOID*)&pSaVar );
            IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "SafeArrayAccessData" );

            pSaVar->vt = VT_BSTR;
            pSaVar->bstrVal = bstr.Detach();

            hr = SafeArrayUnaccessData( pSaStrings );
            IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "SafeArrayUnaccessData" );

            hr = pDocument->write( pSaStrings );
            IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "write" );
        }

        hr = pDocument->close();
        IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "close" );

        hr = pHtaWindow->focus();
        IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "focus" );

        return std::unique_ptr<JsHtmlWindow>( new JsHtmlWindow( cx, pHtaWindow ) );
    }
    catch ( const _com_error& e )
    {
        pfc::string8_fast errorMsg8 = pfc::stringcvt::string_utf8_from_wide( (const wchar_t*)e.ErrorMessage() );
        pfc::string8_fast errorSource8 = pfc::stringcvt::string_utf8_from_wide( e.Source().length() ? (const wchar_t*)e.Source() : L"" );
        pfc::string8_fast errorDesc8 = pfc::stringcvt::string_utf8_from_wide( e.Description().length() ? (const wchar_t*)e.Description() : L"" );
        JS_ReportErrorUTF8( cx, "COM error: message %s; source: %s; description: %s", errorMsg8.c_str(), errorSource8.c_str(), errorDesc8.c_str() );
        return nullptr;
    }
}

size_t JsHtmlWindow::GetInternalSize( const std::wstring& htmlCode, const std::wstring& data, JS::HandleValue callback )
{
    return htmlCode.length() * sizeof( wchar_t ) + data.length() * sizeof( wchar_t );
}

std::optional<nullptr_t>
JsHtmlWindow::Close()
{    
    pHtaWindow_->close();
    return nullptr;
}

}
