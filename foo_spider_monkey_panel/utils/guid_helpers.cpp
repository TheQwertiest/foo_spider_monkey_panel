#include <stdafx.h>

#include "guid_helpers.h"

namespace smp::utils
{

GUID GenerateGuid()
{
    GUID guid{};
    (void)CoCreateGuid( &guid ); //< should not fail
    return guid;
}

std::wstring GuidToStr( const GUID& guid )
{
    std::wstring guidStr;

    guidStr.resize( 64 );
    const auto strSizeWithTerminator = StringFromGUID2( guid, guidStr.data(), guidStr.size() );
    guidStr.resize( strSizeWithTerminator - 1 );

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

size_t GuidHasher::operator()( const GUID& guid ) const
{
    const uint64_t guid64_1 =
        ( static_cast<uint64_t>( guid.Data1 ) << 32 )
        | ( static_cast<uint64_t>( guid.Data2 ) << 16 )
        | guid.Data3;
    uint64_t guid64_2;
    memcpy( &guid64_2, guid.Data4, sizeof( guid.Data4 ) );

    std::hash<std::uint64_t> hash;
    return hash( guid64_1 ) ^ hash( guid64_2 );
}

} // namespace smp::utils
