#include <stdafx.h>
#include "winapi_error_helper.h"

#include <js_utils/scope_helper.h>

namespace mozjs
{

DWORD Win32FromHResult( HRESULT hr )
{
    if ( (hr & 0xFFFF0000) == MAKE_HRESULT( SEVERITY_ERROR, FACILITY_WIN32, 0 ) )
    {
        return HRESULT_CODE( hr );
    }

    if ( hr == S_OK )
    {
        return ERROR_SUCCESS;
    }

    // Not a Win32 HRESULT so return a generic error code.
    return ERROR_CAN_NOT_COMPLETE;
}

pfc::string8_fast MessageFromErrorCode( DWORD errorCode )
{
    LPVOID lpMsgBuf;   

    DWORD dwRet = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errorCode,
        MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ),
        (LPTSTR)&lpMsgBuf,
        0, nullptr );

    if ( !dwRet )
    {
        return pfc::string8_fast();
    }

    scope::unique_ptr<std::remove_pointer_t<LPVOID>> scopedMsg( lpMsgBuf, []( LPVOID buf )
    {
        LocalFree( buf );
    } );

    pfc::string8_fast msg8 = pfc::stringcvt::string_utf8_from_wide( (const wchar_t*)lpMsgBuf );
    return msg8;
}

}
