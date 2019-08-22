#include <stdafx.h>
#include "string_helpers.h"

namespace
{

// TODO: remove tabs as well (string_view, isspace, manual looping?)

template <typename T>
T TrimImpl( const T& str )
{
    size_t first = str.find_first_not_of( ' ' );
    if ( std::string::npos == first )
    {
        return str;
    }
    size_t last = str.find_last_not_of( ' ' );
    return str.substr( first, ( last - first + 1 ) );
}

template <>
pfc::string8_fast TrimImpl( const pfc::string8_fast& str )
{
    size_t first = 0;
    for ( first = 0; first < str.length(); ++first )
    {
        if ( str[first] != ' ' )
        {
            break;
        }
    }

    int32_t last = 0;
    for ( last = static_cast<int32_t>( str.length() ) - 1; last >= 0; --last )
    {
        if ( str[last] != ' ' )
        {
            break;
        }
    }

    if ( ( first == 0 ) && last == ( str.length() - 1 ) )
    {
        return str;
    }

    return pfc::string8_fast( &str[first], last - first + 1 );
}

} // namespace

namespace smp::string
{

std::string Trim( const std::string& str )
{
    return TrimImpl( str );
}

std::wstring Trim( const std::wstring& str )
{
    return TrimImpl( str );
}

pfc::string8_fast Trim( const pfc::string8_fast& str )
{
    return TrimImpl( str );
}

std::vector<std::u8string_view> SplitByLines( std::u8string_view str )
{
    std::vector<std::u8string_view> lines;
    for ( std::u8string_view curScope = str; !curScope.empty(); )
    {
        if ( size_t pos = curScope.find_first_of( "\r\n" );
             std::string::npos != pos )
        {
            if ( pos )
            {
                lines.emplace_back( curScope.data(), pos );
                curScope.remove_prefix( pos );
            }

            while ( !curScope.empty() && ( curScope[0] == '\r' || curScope[0] == '\n' ) )
            {
                curScope.remove_prefix( 1 );
            }
        }
        else
        {
            lines.emplace_back( curScope.data(), curScope.size() );
            curScope = std::u8string_view{};
        }
    }

    return lines;
}

} // namespace smp::string
