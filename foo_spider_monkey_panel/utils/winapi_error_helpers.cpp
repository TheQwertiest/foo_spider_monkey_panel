#include <stdafx.h>
#include "winapi_error_helpers.h"

#include <utils/scope_helpers.h>
#include <utils/string_helpers.h>

using namespace smp;

namespace
{

pfc::string8_fast MessageFromErrorCode( DWORD errorCode )
{
    LPVOID lpMsgBuf;

    DWORD dwRet = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errorCode,
        MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ),
        (LPTSTR)&lpMsgBuf,
        0,
        nullptr );
    if ( !dwRet )
    {
        return pfc::string8_fast();
    }

    utils::final_action autoMsg( [lpMsgBuf] {
        LocalFree( lpMsgBuf );
    } );

    return pfc::stringcvt::string_utf8_from_wide( (const wchar_t*)lpMsgBuf ).get_ptr();    
}

} // namespace

namespace smp::error
{

void CheckHR( HRESULT hr, std::string_view functionName )
{
    CheckWinApi( SUCCEEDED( hr ), functionName );
}

void CheckWinApi( bool checkValue, std::string_view functionName )
{
    if ( !checkValue )
    {
        const DWORD errorCode = GetLastError();
        throw SmpException( fmt::format( "WinAPI error: {} failed with error ({:#X}): {}", functionName, errorCode, MessageFromErrorCode( errorCode ).c_str() ) );
    }
}

} // namespace smp::error
