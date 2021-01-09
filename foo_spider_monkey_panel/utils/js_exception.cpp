#include <stdafx.h>

#include "js_exception.h"

namespace smp
{

_Post_satisfies_( checkValue ) void JsException::ExpectTrue( bool checkValue )
{
    if ( !checkValue )
    {
        throw JsException();
    }
}

void JsException::ExpectTrue( _Post_notnull_ void* checkValue )
{
    return ExpectTrue( static_cast<bool>( checkValue ) );
}

} // namespace smp
