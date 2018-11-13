#include <stdafx.h>
#include "smp_exception.h"

namespace smp
{

void SmpException::ExpectTrue( bool checkValue, std::string_view errorMessage )
{
    if ( !checkValue )
    {
        throw SmpException( std::string( errorMessage.data(), errorMessage.size() ) );
    }
}

void JsException::ExpectTrue( bool checkValue )
{
    if ( !checkValue )
    {
        throw JsException();
    }
}

} // namespace smp
