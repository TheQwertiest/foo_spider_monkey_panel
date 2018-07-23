#include <stdafx.h>
#include "string_util.h"

namespace
{

template <typename T>
std::remove_const_t<std::remove_reference_t<T>> TrimImpl(const T& str)
{
    size_t first = str.find_first_not_of( ' ' );
    if ( std::string::npos == first )
    {
        return str;
    }
    size_t last = str.find_last_not_of( ' ' );
    return str.substr( first, (last - first + 1) );
}

}

namespace mozjs
{

std::string Trim( const std::string& str )
{
    return TrimImpl( str );
}

std::wstring Trim( const std::wstring& str )
{
    return TrimImpl( str );
}

}
