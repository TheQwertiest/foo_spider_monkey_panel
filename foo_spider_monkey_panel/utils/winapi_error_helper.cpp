#include <stdafx.h>
#include "winapi_error_helper.h"

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

    pfc::string8_fast msg8 = pfc::stringcvt::string_utf8_from_wide( (const wchar_t*)lpMsgBuf );
    return msg8;
}

} // namespace

namespace smp::error
{

void CheckHR( HRESULT hr, std::string_view functionName )
{
    if ( FAILED( hr ) )
    {
        pfc::string8_fast errorStr = MessageFromErrorCode( hr );
        throw SmpException(
            smp::string::Formatter()
            << "WinAPI error: " << std::string( functionName.data(), functionName.size() )
            << " failed with error (0x" << std::hex << hr << ": " << errorStr.c_str() );
    }
}

void CheckWinApi( bool checkValue, std::string_view functionName )
{
    if ( !checkValue )
    {
        DWORD errorCode = GetLastError();
        pfc::string8_fast errorStr = MessageFromErrorCode( errorCode );
        throw SmpException(
            smp::string::Formatter()
            << "WinAPI error: " << std::string( functionName.data(), functionName.size() )
            << " failed with error (0x" << std::hex << errorCode << ": " << errorStr.c_str() );
    }
}

} // namespace smp::error
