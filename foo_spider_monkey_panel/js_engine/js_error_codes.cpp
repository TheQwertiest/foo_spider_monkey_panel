#include <stdafx.h>

#include "js_error_codes.h"

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#include <jsfriendapi.h>
#pragma warning( pop )  



namespace mozjs
{

std::string GetCurrentExceptionText( JSContext* cx )
{
    if ( !JS_IsExceptionPending( cx ) )
    {
        return std::string();
    }

    // Get exception object before printing and clearing exception.
    JS::RootedValue excn( cx );
    (void)JS_GetPendingException( cx, &excn );

    JS::RootedObject global( cx, JS::CurrentGlobalOrNull( cx ) );
    if ( !global )
    {
        return std::string();
    }

    JS::RootedObject excnObject( cx, excn.toObjectOrNull() );
    if ( !excnObject )
    {
        return std::string();
    }

    JSErrorReport* report = JS_ErrorFromException( cx, excnObject );
    if ( !report )
    {
        return std::string();
    }

    if ( JSMSG_USER_DEFINED_ERROR != report->errorNumber )
    {
        return std::string();
    }

    JS_ClearPendingException( cx );
    return report->message().c_str();
}

}
