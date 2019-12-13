#include <stdafx.h>

#include "winapi_error_helpers.h"

#include <utils/scope_helpers.h>
#include <utils/string_helpers.h>

using namespace smp;

namespace
{

/// @remark `std::system_category().message(errorCode)` is unsuitable, since it localizes the message
///         and often in non-unicode way (while JS engine supports only UTF-8 and ASCII).
std::u8string MessageFromErrorCode( DWORD errorCode )
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
        return std::u8string{ "Unknown error" };
    }
    assert( lpMsgBuf );

    utils::final_action autoMsg( [lpMsgBuf] {
        LocalFree( lpMsgBuf );
    } );

    return smp::unicode::ToU8( std::wstring_view{ reinterpret_cast<const wchar_t*>( lpMsgBuf ) } );
}

void ThrowParsedWinapiError( DWORD errorCode, std::string_view functionName )
{
    throw SmpException( fmt::format( "WinAPI error: {} failed with error ({:#x}): {}", functionName, errorCode, MessageFromErrorCode( errorCode ) ) );
}

} // namespace

namespace smp::error
{

#pragma warning( push )
#pragma warning( disable : 28196 ) // The expression does not evaluate to true

_Post_satisfies_( SUCCEEDED( hr ) )
void CheckHR( HRESULT hr, std::string_view functionName )
{
    if ( FAILED( hr ) )
    {
        ThrowParsedWinapiError( hr, functionName );
    }
}

_Post_satisfies_( checkValue )
void CheckWinApi( bool checkValue, std::string_view functionName )
{
    if ( !checkValue )
    {
        const DWORD errorCode = GetLastError();
        ThrowParsedWinapiError( errorCode, functionName );
    }
}

#pragma warning( pop )

void CheckWinApi( _Post_notnull_ void* checkValue, std::string_view functionName )
{
    return CheckWinApi( static_cast<bool>( checkValue ), functionName );
}

} // namespace smp::error
