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

#include <helpers.h>

#pragma warning( push )
#pragma warning( disable : 4192 )
#pragma warning( disable : 4146 )
#pragma warning( disable : 4278 )
#import <mshtml.tlb>
#import <shdocvw.dll>
#undef GetWindowStyle
#undef GetWindowLong
#undef GetFreeSpace
#import <wshom.ocx>
#pragma warning( pop )

// std::time
#include <ctime>
#include <filesystem>


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
    JSCLASS_HAS_PRIVATE | JSCLASS_FOREGROUND_FINALIZE, // COM objects must be finalized in foreground
    &jsOps
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsHtmlWindow, Close )
MJS_DEFINE_JS_TO_NATIVE_FN( JsHtmlWindow, Focus )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "Close", Close, 0, DefaultPropsFlags() ),
    JS_FN( "Focus", Focus, 0, DefaultPropsFlags() ),
    JS_FS_END
};

const JSPropertySpec jsProperties[] = {
    JS_PS_END
};

} // namespace

namespace mozjs
{

const JSClass JsHtmlWindow::JsClass = jsClass;
const JSFunctionSpec* JsHtmlWindow::JsFunctions = jsFunctions;
const JSPropertySpec* JsHtmlWindow::JsProperties = jsProperties;
const JsPrototypeId JsHtmlWindow::PrototypeId = JsPrototypeId::HtmlWindow;

JsHtmlWindow::JsHtmlWindow( JSContext* cx, DWORD pid, HtmlWindow2ComPtr pWindow )
    : pJsCtx_( cx )
    , pid_( pid )
    , pWindow_( pWindow )
{
}

JsHtmlWindow::~JsHtmlWindow()
{
    Close();
}

// TODO: cleanup the code
// TODO: add html window centering

std::unique_ptr<JsHtmlWindow>
JsHtmlWindow::CreateNative( JSContext* cx, const std::wstring& htmlCode, JS::HandleValue options )
{
    uint32_t width = 400;
    uint32_t height = 400;
    std::wstring title = L"foobar2000";
    bool isContextMenuEnabled = false;
    _variant_t storedData;
    JS::RootedFunction storedFunction( cx );

    if ( !options.isNullOrUndefined() )
    {
        if ( !options.isObject() )
        {
            JS_ReportErrorUTF8( cx, "options argument is not an object" );
            return nullptr;
        }

        JS::RootedObject jsObject( cx, &options.toObject() );
        bool hasProp;
        if ( !JS_HasProperty( cx, jsObject, "width", &hasProp ) )
        { // reports
            return nullptr;
        }

        if ( hasProp )
        {
            JS::RootedValue jsValue( cx );
            if ( !JS_GetProperty( cx, jsObject, "width", &jsValue ) )
            { // reports
                return nullptr;
            }

            auto retVal = convert::to_native::ToValue<uint32_t>( cx, jsValue );
            if ( !retVal )
            {
                JS_ReportErrorUTF8( cx, "`width` can't be converted to uint32_t" );
                return nullptr;
            }

            width = retVal.value();
        }

        if ( !JS_HasProperty( cx, jsObject, "height", &hasProp ) )
        { // reports
            return nullptr;
        }

        if ( hasProp )
        {
            JS::RootedValue jsValue( cx );
            if ( !JS_GetProperty( cx, jsObject, "height", &jsValue ) )
            { // reports
                return nullptr;
            }

            auto retVal = convert::to_native::ToValue<uint32_t>( cx, jsValue );
            if ( !retVal )
            {
                JS_ReportErrorUTF8( cx, "`height` can't be converted to uint32_t" );
                return nullptr;
            }

            height = retVal.value();
        }

        if ( !JS_HasProperty( cx, jsObject, "title", &hasProp ) )
        { // reports
            return nullptr;
        }

        if ( hasProp )
        {
            JS::RootedValue jsValue( cx );
            if ( !JS_GetProperty( cx, jsObject, "title", &jsValue ) )
            { // reports
                return nullptr;
            }

            auto retVal = convert::to_native::ToValue<std::wstring>( cx, jsValue );
            if ( !retVal )
            {
                JS_ReportErrorUTF8( cx, "`title` can't be converted to uint32_t" );
                return nullptr;
            }

            title = retVal.value();
        }

        if ( !JS_HasProperty( cx, jsObject, "context_menu", &hasProp ) )
        { // reports
            return nullptr;
        }

        if ( hasProp )
        {
            JS::RootedValue jsValue( cx );
            if ( !JS_GetProperty( cx, jsObject, "context_menu", &jsValue ) )
            { // reports
                return nullptr;
            }

            auto retVal = convert::to_native::ToValue<bool>( cx, jsValue );
            if ( !retVal )
            {
                JS_ReportErrorUTF8( cx, "`context_menu` can't be converted to bool" );
                return nullptr;
            }

            isContextMenuEnabled = retVal.value();
        }

        if ( !JS_HasProperty( cx, jsObject, "data", &hasProp ) )
        { // reports
            return nullptr;
        }

        if ( hasProp )
        {
            JS::RootedValue jsValue( cx );
            if ( !JS_GetProperty( cx, jsObject, "data", &jsValue ) )
            { // reports
                return nullptr;
            }

            if ( !convert::com::JsToVariant( cx, jsValue, *storedData.GetAddress() ) )
            {
                JS_ReportErrorUTF8( cx, "`data` is of unsupported type" );
                return nullptr;
            }
        }

        if ( !JS_HasProperty( cx, jsObject, "fn", &hasProp ) )
        { // reports
            return nullptr;
        }

        if ( hasProp )
        {
            JS::RootedValue jsValue( cx );
            if ( !JS_GetProperty( cx, jsObject, "fn", &jsValue ) )
            { // reports
                return nullptr;
            }

            if ( !( jsValue.isObject() && JS_ObjectIsFunction( cx, &jsValue.toObject() ) )
                 && !jsValue.isNullOrUndefined() )
            {
                JS_ReportErrorUTF8( cx, "fn argument is not a function" );
                return nullptr;
            }

            if ( jsValue.isObject() )
            {
                storedFunction.set( JS_ValueToFunction( cx, jsValue ) );
            }
        }
    }

    try
    {
        const auto wndId = [] {
            std::srand( unsigned( std::time( 0 ) ) );
            return L"a" + std::to_wstring( std::rand() );
        }();

        const std::wstring features = [isContextMenuEnabled] 
        {
            std::wstring features = L"singleinstance=yes "
                                    L"border=dialog "
                                    L"minimizeButton=no "
                                    L"maximizeButton=no "
                                    L"scroll=no "
                                    L"showintaskbar=yes "
                                    L"contextMenu=yes "
                                    L"innerBorder=no";

            features += (isContextMenuEnabled ? L" selection=yes" : L" selection=no");

            namespace fs = std::filesystem;

            const pfc::string8_fast path = helpers::get_fb2k_path() + "foobar2000.exe";
            fs::path fsPath = fs::u8path( path.c_str() );
            std::error_code dummyErr;
            if ( fs::exists( fsPath ) && fs::is_regular_file( fsPath, dummyErr ) )
            {
                features += L" icon=\"" + fsPath.wstring() + L"\"";
            }

            return features;            
        }();

        const std::wstring htaCode = 
            L"<script>moveTo(-1000,-1000);resizeTo(0,0);</script>"
            L"<hta:application id=app " + features + L" />"
            L"<object id='" + wndId + L"' style='display:none' classid='clsid:8856F961-340A-11D0-A96B-00C04FD705A2'>"
            L"    <param name=RegisterAsBrowser value=1>"
            L"</object>";

        const std::wstring launchCmd = L"\"about:" + htaCode + L"\"";

        SHELLEXECUTEINFO execInfo = { 0 };
        execInfo.cbSize = sizeof( execInfo );
        execInfo.lpVerb = L"open";
        execInfo.lpFile = L"mshta.exe";
        execInfo.lpParameters = launchCmd.c_str();
        execInfo.fMask = SEE_MASK_NOCLOSEPROCESS;

        BOOL bRet = ShellExecuteEx( &execInfo );
        IF_WINAPI_FAILED_RETURN_WITH_REPORT( cx, bRet && ( (DWORD)execInfo.hInstApp > 32 ), nullptr, ShellExecuteEx );

        DWORD pid = GetProcessId( execInfo.hProcess );

        // TODO: add incremental sleep
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
        { // Ideally we should use HWND instead, but GetHWND fails for no apparent reason...
            _variant_t va( i, VT_I4 );
            CDispatchPtr pCurWindow = pShellWindows->Item( va );
            _variant_t idVal = pCurWindow.Get( L"id" );
            if ( idVal.vt == VT_EMPTY )
            {
                continue;
            }

            if ( static_cast<_bstr_t>( idVal ) != _bstr_t( wndId.c_str() ) )
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

        scope::final_action autoWindow( [&pHtaWindow] {
            pHtaWindow->close();
        } );

        IDispatchExPtr pHtaWindowEx( pHtaWindow );
        assert( !!pHtaWindowEx );

        { // open document
            SAFEARRAY* pSaStrings = SafeArrayCreateVector( VT_VARIANT, 0, 1 );
            scope::final_action autoPsa( [pSaStrings]() {
                SafeArrayDestroy( pSaStrings );
            } );

            VARIANT* pSaVar = nullptr;
            hr = SafeArrayAccessData( pSaStrings, (LPVOID*)&pSaVar );
            IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "SafeArrayAccessData" );

            // A dirty, dirty, dirty, dirty hack, to force IE8 mode.
            // The first `write` sets IE compatibility mode, but we can't
            // use it to write user html code, since we need to set `stored` data
            // before that. Which can't be set unless document is open.
            // Which can only be opened with `write` method.
            // Thus, the hack (thank God for IE compatibility with such malformed html).
            _bstr_t bstr( L"<html><head><meta http-equiv=\"x-ua-compatible\" content=\"IE=8\" /></head></html>" );
            pSaVar->vt = VT_BSTR;
            pSaVar->bstrVal = bstr.Detach();

            hr = SafeArrayUnaccessData( pSaStrings );
            IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "SafeArrayUnaccessData" );

            hr = pDocument->write( pSaStrings );
            IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "write" );
        }

        { // store callback data
            DISPID dispId;
            _bstr_t varName = L"stored_data";
            hr = pHtaWindowEx->GetDispID( varName.GetBSTR(), fdexNameEnsure, &dispId );
            IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "GetDispID" );

            DISPID dispPut = { DISPID_PROPERTYPUT };
            DISPPARAMS dispParams = { &storedData.GetVARIANT(), &dispPut, 1, 1 };

            hr = pHtaWindowEx->InvokeEx( dispId, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT, &dispParams, nullptr, nullptr, nullptr );
            IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "InvokeEx" );
        }

        if ( storedFunction )
        { // store callback function
            DISPID dispId;
            _bstr_t varName = L"stored_function";
            hr = pHtaWindowEx->GetDispID( varName.GetBSTR(), fdexNameEnsure, &dispId );
            IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "GetDispID" );

            _variant_t callbackFnV;
            JS::RootedValue jsValue( cx, JS::ObjectValue( *JS_GetFunctionObject( storedFunction ) ) );
            convert::com::JsToVariant( cx, jsValue, callbackFnV.GetVARIANT() );

            DISPID dispPut = { DISPID_PROPERTYPUT };
            DISPPARAMS dispParams = { &callbackFnV.GetVARIANT(), &dispPut, 1, 1 };

            hr = pHtaWindowEx->InvokeEx( dispId, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT, &dispParams, nullptr, nullptr, nullptr );
            IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "InvokeEx" );
        }

        { // Update window according to options
            _bstr_t bTitle( title.c_str() );
            hr = pDocument->put_title( bTitle.Detach() );
            IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "put_title" );

            hr = pHtaWindow->resizeTo( width, height );
            IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "resizeTo" );

            // TODO: change to relative to fb2k center
            hr = pHtaWindow->moveTo( ( pHtaWindow->screen->width - width ) / 2, ( pHtaWindow->screen->height - height ) / 2 );
            IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "moveTo" );
        }

        { // open document
            SAFEARRAY* pSaStrings = SafeArrayCreateVector( VT_VARIANT, 0, 1 );
            scope::final_action autoPsa( [pSaStrings]() {
                SafeArrayDestroy( pSaStrings );
            } );

            VARIANT* pSaVar = nullptr;
            hr = SafeArrayAccessData( pSaStrings, (LPVOID*)&pSaVar );
            IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "SafeArrayAccessData" );

            _bstr_t bstr( htmlCode.c_str() );
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

        autoWindow.cancel();
        return std::unique_ptr<JsHtmlWindow>( new JsHtmlWindow( cx, pid, pHtaWindow ) );
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

size_t JsHtmlWindow::GetInternalSize( const std::wstring& htmlCode, JS::HandleValue options )
{
    return htmlCode.length() * sizeof( wchar_t );
}

std::optional<nullptr_t>
JsHtmlWindow::Close()
{
    if ( !pid_ )
    {
        return nullptr;
    }

    HANDLE hProc = OpenProcess( SYNCHRONIZE | PROCESS_TERMINATE, FALSE, pid_ );
    if ( !hProc )
    {
        return nullptr;
    }

    auto enumFn = []( HWND hwnd, LPARAM lParam ) -> BOOL {
        DWORD pid;
        GetWindowThreadProcessId( hwnd, &pid );

        if ( pid == (DWORD)lParam )
        {
            PostMessage( hwnd, WM_CLOSE, 0, 0 );
        }

        return TRUE;
    };

    // TerminateAppEnum() posts WM_CLOSE to all windows whose PID
    // matches your process's.
    EnumWindows( (WNDENUMPROC)enumFn, (LPARAM)pid_ );

    // Wait on the handle. If it signals, great. If it times out,
    // then you kill it.
    if ( WaitForSingleObject( hProc, 1000 ) != WAIT_OBJECT_0 )
    {
        TerminateProcess( hProc, 0 );
    }

    CloseHandle( hProc );

    pid_ = 0;
    pWindow_.Release();

    return nullptr;
}

std::optional<nullptr_t> JsHtmlWindow::Focus()
{
    if ( !pid_ || !pWindow_ )
    {
        return nullptr;
    }
    HANDLE hProc = OpenProcess( PROCESS_QUERY_INFORMATION, FALSE, pid_ );
    if ( !hProc )
    {
        return nullptr;
    }
    scope::final_action autoProc( [&hProc] {
        CloseHandle( hProc );
    } );

    DWORD exitCode;
    if ( !GetExitCodeProcess( hProc, &exitCode ) || STILL_ACTIVE != exitCode )
    {
        return nullptr;
    }

    HRESULT hr = pWindow_->focus();
    IF_HR_FAILED_RETURN_WITH_REPORT( pJsCtx_, hr, nullptr, "focus" );

    return nullptr;
}

} // namespace mozjs
