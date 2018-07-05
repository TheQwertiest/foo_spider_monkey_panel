#include <stdafx.h>
#include "string_util.h"

namespace mozjs
{

std::string Trim( const std::string& str )
{
    size_t first = str.find_first_not_of( ' ' );
    if ( std::string::npos == first )
    {
        return str;
    }
    size_t last = str.find_last_not_of( ' ' );
    return str.substr( first, (last - first + 1) );
}

std::wstring Trim( const std::wstring& str )
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
