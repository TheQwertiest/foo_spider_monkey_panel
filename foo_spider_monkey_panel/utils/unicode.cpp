#include <stdafx.h>

#include "unicode.h"

namespace smp::unicode
{

std::wstring ToWide( std::u8string_view src )
{
    if ( src.empty() )
    {
        return std::wstring{};
    }

    size_t stringLen = MultiByteToWideChar( CP_UTF8, 0, src.data(), src.size(), nullptr, 0 );
    std::wstring strVal;
    strVal.resize( stringLen );

    stringLen = MultiByteToWideChar( CP_UTF8, 0, src.data(), src.size(), strVal.data(), strVal.size() );
    strVal.resize( stringLen );

    return strVal;
}

std::wstring ToWide( const pfc::string_base& src )
{
    return ToWide( std::u8string_view{ src.c_str(), src.length() } );
}

std::wstring ToWide_FromAcp( std::string_view src )
{
    if ( src.empty() )
    {
        return std::wstring{};
    }

    size_t stringLen = MultiByteToWideChar( CP_ACP, 0, src.data(), src.size(), nullptr, 0 );
    std::wstring strVal;
    strVal.resize( stringLen );

    stringLen = MultiByteToWideChar( CP_ACP, 0, src.data(), src.size(), strVal.data(), strVal.size() );
    strVal.resize( stringLen );

    return strVal;
}

std::u8string ToU8( std::wstring_view src )
{
    if ( src.empty() )
    {
        return std::u8string{};
    }

    size_t stringLen = WideCharToMultiByte( CP_UTF8, 0, src.data(), src.size(), nullptr, 0, nullptr, nullptr );
    std::u8string strVal;
    strVal.resize( stringLen );

    stringLen = WideCharToMultiByte( CP_UTF8, 0, src.data(), src.size(), strVal.data(), strVal.size(), nullptr, nullptr );
    strVal.resize( stringLen );

    return strVal;
}

std::u8string ToU8_FromAcpToWide( std::string_view src )
{
    return ToU8( ToWide_FromAcp( src ) );
}

} // namespace smp::unicode
