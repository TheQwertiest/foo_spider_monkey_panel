#include <stdafx.h>
#include "smp_exception.h"

namespace smp
{

 _Post_satisfies_( checkValue ) 
void SmpException::ExpectTrue( bool checkValue, std::string_view errorMessage )
{
    if ( !checkValue )
    {
        throw SmpException( std::string( errorMessage.data(), errorMessage.size() ) );
    }
}

void SmpException::ExpectTrue( _Post_notnull_ void* checkValue, std::string_view errorMessage )
{
    return ExpectTrue( static_cast<bool>( checkValue ), errorMessage );
}

 _Post_satisfies_( checkValue ) 
void JsException::ExpectTrue( bool checkValue )
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
