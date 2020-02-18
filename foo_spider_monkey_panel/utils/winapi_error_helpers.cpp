#include <stdafx.h>

#include "winapi_error_helpers.h"

#include <utils/scope_helpers.h>
#include <utils/string_helpers.h>

using namespace smp;

namespace
{

std::u8string MessageFromErrorCode( DWORD errorCode )
{
    return smp::unicode::ToU8_FromAcpToWide( std::system_category().message( errorCode ) );
}

void ThrowParsedWinapiError( DWORD errorCode, std::string_view functionName )
{
    throw SmpException( fmt::format( "WinAPI error:\n"
                                     "  {} failed with error ({:#x}):\n"
                                     "    {}",
                                     functionName,
                                     errorCode,
                                     MessageFromErrorCode( errorCode ) ) );
}

} // namespace

namespace smp::error
{

#pragma warning( push )
#pragma warning( disable : 28196 ) // The expression does not evaluate to true

_Post_satisfies_( SUCCEEDED( hr ) ) void CheckHR( HRESULT hr, std::string_view functionName )
{
    if ( FAILED( hr ) )
    {
        ThrowParsedWinapiError( hr, functionName );
    }
}

_Post_satisfies_( checkValue ) void CheckWinApi( bool checkValue, std::string_view functionName )
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
