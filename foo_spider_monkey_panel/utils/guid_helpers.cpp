#include <stdafx.h>

#include "guid_helpers.h"

namespace smp::utils
{

GUID GenerateGuid()
{
    GUID guid;
    (void)CoCreateGuid( &guid ); //< should not fail
    return guid;
}

std::wstring GuidToStr( const GUID& guid )
{
    std::wstring guidStr;

    guidStr.resize( 64 );
    StringFromGUID2( guid, guidStr.data(), guidStr.size() );
    guidStr.resize( wcslen( guidStr.c_str() ) );

    return guidStr;
}

std::optional<GUID> StrToGuid( const std::wstring& str )
{
    GUID guid;
    HRESULT hr = IIDFromString( str.c_str(), &guid );
    if ( FAILED( hr ) )
    {
        return std::nullopt;
    }

    return guid;
}

} // namespace smp::utils
