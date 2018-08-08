#include <stdafx.h>
#include "html_window.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/gdi_error_helper.h>
#include <js_utils/winapi_error_helper.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>

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

const JSFunctionSpec jsFunctions[] = {
    JS_FS_END
};

//MJS_DEFINE_JS_TO_NATIVE_FN( JsHtmlWindow, get_Height )

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

JsHtmlWindow::JsHtmlWindow( JSContext* cx)
    : pJsCtx_( cx )
{
}

JsHtmlWindow::~JsHtmlWindow()
{ 
}

std::unique_ptr<JsHtmlWindow> 
JsHtmlWindow::CreateNative( JSContext* cx, const std::wstring& htmlCode, const std::wstring& data, JS::HandleValue callback )
{
    try
    {
        IShellDispatchPtr pShell;
        HRESULT hr = pShell.GetActiveObject( CLSID_Shell );
        if ( FAILED( hr ) )
        {
            hr = pShell.CreateInstance( CLSID_Shell, nullptr, CLSCTX_INPROC_SERVER );
            IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "CreateInstance" );
        }

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

        /*
        HINSTANCE dwRet = ShellExecute( nullptr, L"open", L"mshta.exe", htaCode.c_str(), nullptr, 1 );
        if ( (DWORD)dwRet <= 32 )
        {// yep, WinAPI logic
            JS_ReportErrorUTF8( cx, "ShellExecute failed: %u", dwRet );
            return nullptr;
        }
        */

        IWshRuntimeLibrary::IWshShellPtr pWsh;
        hr = pWsh.GetActiveObject( L"WScript.Shell" );
        if ( FAILED( hr ) )
        {
            hr = pWsh.CreateInstance( L"WScript.Shell", nullptr, CLSCTX_INPROC_SERVER );
            IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "CreateInstance" );
        }

        const std::wstring cmd = L"mshta.exe about:" + htaCode;
        hr = pWsh->Run( cmd.c_str() );
        IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "Run" );

        IDispatchPtr pDispatch;
        hr = pShell->Windows( &pDispatch );
        IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "Windows" );

        SHDocVw::IShellWindowsPtr pShellWindows( pDispatch );

        long windowsCount = pShellWindows->GetCount();

        for ( long i = 0; i < windowsCount; ++i )
        {
            Sleep(100);

            _variant_t va( i, VT_I4 );           
            SHDocVw::IWebBrowser2Ptr pCurWindow = pShellWindows->Item( va );               
            MSHTML::IHTMLDocument2Ptr pParentDoc = pCurWindow->GetParent();    
            if ( !pParentDoc )
            {
                continue;
                //JS_ReportErrorUTF8( cx, "Failed to get IHTMLDocument2" );
                //return nullptr;
            }
            MSHTML::IHTMLWindow2Ptr pHtaWindow = pParentDoc->GetparentWindow();
            if ( !pHtaWindow )
            {
                JS_ReportErrorUTF8( cx, "Failed to get IHTMLWindow2" );
                return nullptr;
            }
        }
    }
    catch ( const _com_error& e )
    {
        pfc::string8_fast errorMsg8 = pfc::stringcvt::string_utf8_from_wide( (const wchar_t*)e.ErrorMessage() );        
        pfc::string8_fast errorSource8 = pfc::stringcvt::string_utf8_from_wide( (const wchar_t*)e.Source() );
        pfc::string8_fast errorDesc8 = pfc::stringcvt::string_utf8_from_wide( (const wchar_t*)e.Description() );
        JS_ReportErrorUTF8( cx, "COM error: message %s; source: %s; description: %s", errorMsg8.c_str(), errorSource8.c_str(), errorDesc8.c_str() );
        return nullptr;
    }

    return std::unique_ptr<JsHtmlWindow>( new JsHtmlWindow(cx ) );
}

size_t JsHtmlWindow::GetInternalSize( const std::wstring& htmlCode, const std::wstring& data, JS::HandleValue callback )
{
    return htmlCode.length() * sizeof( wchar_t ) + data.length() * sizeof( wchar_t );
}

}
