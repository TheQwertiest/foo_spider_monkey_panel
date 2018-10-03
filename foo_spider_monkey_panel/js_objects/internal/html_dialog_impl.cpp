#include <stdafx.h>
#include "html_dialog_impl.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/winapi_error_helper.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/scope_helper.h>
#include <utils/timer_helpers.h>
#include <convert/com.h>

#include <com_objects/html_moniker.h>

#include <helpers.h>

#pragma warning( push )
#pragma warning( disable : 4192 )
#pragma warning( disable : 4146 )
#pragma warning( disable : 4278 )
#import <mshtml.tlb>
#pragma warning( pop )

// std::time
#include <ctime>

_COM_SMARTPTR_TYPEDEF( IMoniker, __uuidof( IMoniker ) );
_COM_SMARTPTR_TYPEDEF( IHostDialogHelper, __uuidof(IHostDialogHelper) );


namespace mozjs
{
std::optional<JS::Value>
ShowHtmlDialogImpl( JSContext* cx, uint32_t hWnd, const std::wstring& htmlCode, JS::HandleValue options )
{
    if ( !hWnd )
    {
        JS_ReportErrorUTF8( cx, "Invalid window ID: %u", hWnd );
        return std::nullopt;
    }

    uint32_t width = 400;
    uint32_t height = 400;
    bool isModal = true;
    _variant_t storedData;

    if ( !options.isNullOrUndefined() )
    {
        if ( !options.isObject() )
        {
            JS_ReportErrorUTF8( cx, "options argument is not an object" );
            return std::nullopt;
        }

        JS::RootedObject jsObject( cx, &options.toObject() );
        bool hasProp;
        if ( !JS_HasProperty( cx, jsObject, "width", &hasProp ) )
        { // reports
            return std::nullopt;
        }

        if ( hasProp )
        {
            JS::RootedValue jsValue( cx );
            if ( !JS_GetProperty( cx, jsObject, "width", &jsValue ) )
            { // reports
                return std::nullopt;
            }

            auto retVal = convert::to_native::ToValue<uint32_t>( cx, jsValue );
            if ( !retVal )
            {
                JS_ReportErrorUTF8( cx, "`width` can't be converted to uint32_t" );
                return std::nullopt;
            }

            width = retVal.value();
        }

        if ( !JS_HasProperty( cx, jsObject, "height", &hasProp ) )
        { // reports
            return std::nullopt;
        }

        if ( hasProp )
        {
            JS::RootedValue jsValue( cx );
            if ( !JS_GetProperty( cx, jsObject, "height", &jsValue ) )
            { // reports
                return std::nullopt;
            }

            auto retVal = convert::to_native::ToValue<uint32_t>( cx, jsValue );
            if ( !retVal )
            {
                JS_ReportErrorUTF8( cx, "`height` can't be converted to uint32_t" );
                return std::nullopt;
            }

            height = retVal.value();
        }

        if ( !JS_HasProperty( cx, jsObject, "modal", &hasProp ) )
        { // reports
            return std::nullopt;
        }

        if ( hasProp )
        {
            JS::RootedValue jsValue( cx );
            if ( !JS_GetProperty( cx, jsObject, "modal", &jsValue ) )
            { // reports
                return std::nullopt;
            }

            auto retVal = convert::to_native::ToValue<bool>( cx, jsValue );
            if ( !retVal )
            {
                JS_ReportErrorUTF8( cx, "`modal` can't be converted to bool" );
                return std::nullopt;
            }

            isModal = retVal.value();
        }

        if ( !JS_HasProperty( cx, jsObject, "data", &hasProp ) )
        { // reports
            return std::nullopt;
        }

        if ( hasProp )
        {
            JS::RootedValue jsValue( cx );
            if ( !JS_GetProperty( cx, jsObject, "data", &jsValue ) )
            { // reports
                return std::nullopt;
            }

            if ( !convert::com::JsToVariant( cx, jsValue, *storedData.GetAddress() ) )
            {
                JS_ReportErrorUTF8( cx, "`data` is of unsupported type" );
                return std::nullopt;
            }
        }
    }

    try
    {
        auto pMoniker = new com_object_impl_t<smp::com::IHtmlMoniker>;
        mozjs::scope::final_action autoMoniker( [&pMoniker] {
            pMoniker->Release();
        } );

        HRESULT hr = pMoniker->SetHTML( htmlCode );
        IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, std::nullopt, SetHTML );

        IHostDialogHelperPtr pHDH;
        hr = pHDH.GetActiveObject( CLSID_HostDialogHelper );
        if ( FAILED( hr ) )
        {
            hr = pHDH.CreateInstance( CLSID_HostDialogHelper, nullptr, CLSCTX_INPROC_SERVER );
            IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, std::nullopt, CreateInstance );
        }

        const std::wstring dialogOptions =
            L"dialogHeight: " + std::to_wstring( height ) + L"px;" + L"dialogWidth: " + std::to_wstring( width ) + L"px;";

        _variant_t dialogRet;
        // Such cast to HWND works only in 32bit app
        hr = pHDH->ShowHTMLDialog( (HWND)hWnd,
                                   pMoniker,
                                   &storedData.GetVARIANT(),
                                   const_cast<WCHAR*>( dialogOptions.c_str() ),
                                   ( isModal ? dialogRet.GetAddress() : nullptr ),
                                   nullptr );
        IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, std::nullopt, ShowHTMLDialog );

        if ( isModal )
        {
            JS::RootedValue jsDialogRet( cx );
            if ( !convert::com::VariantToJs( cx, dialogRet, &jsDialogRet ) )
            {
                JS_ReportErrorUTF8( cx, "Failed to convert dialog return value" );
                return std::nullopt;
            }

            return jsDialogRet;
        }
        else
        {
            return JS::UndefinedValue();
        }
    }
    catch ( const _com_error& e )
    {
        pfc::string8_fast errorMsg8 = pfc::stringcvt::string_utf8_from_wide( (const wchar_t*)e.ErrorMessage() );
        pfc::string8_fast errorSource8 = pfc::stringcvt::string_utf8_from_wide( e.Source().length() ? (const wchar_t*)e.Source() : L"" );
        pfc::string8_fast errorDesc8 = pfc::stringcvt::string_utf8_from_wide( e.Description().length() ? (const wchar_t*)e.Description() : L"" );
        JS_ReportErrorUTF8( cx, "COM error: message %s; source: %s; description: %s", errorMsg8.c_str(), errorSource8.c_str(), errorDesc8.c_str() );
        return std::nullopt;
    }
}

} // namespace mozjs
