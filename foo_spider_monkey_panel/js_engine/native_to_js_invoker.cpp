#include <stdafx.h>
#include "native_to_js_invoker.h"

#include <js_objects/global_object.h>
#include <js_utils/js_error_helper.h>

#pragma warning( push )  
#pragma warning( disable : 4100 ) // unused variable
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4324 ) // structure was padded due to alignment specifier
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#   include <js/Conversions.h>
#pragma warning( pop ) 


namespace
{

uint32_t gcCounter = 0;

}

namespace mozjs
{
bool InvokeJsCallback_Impl( JSContext* cx,
                            JS::HandleObject globalObject,
                            pfc::string8_fast functionName,
                            const JS::HandleValueArray& args,
                            JS::MutableHandleValue rval )
{      
    // TODO: move this to before argument parsing: first check then parse!
    AutoReportException are( cx );

    // TODO: implement a better GC mechanism
    if ( !(gcCounter % 100) )
    {
        JS_MaybeGC( cx );        
    }
    if ( ++gcCounter > 1000 )
    {
        JS_GC( cx );
        gcCounter = 0;
    }
    ++gcCounter;

    JS::RootedValue funcValue( cx );
    if ( !JS_GetProperty( cx, globalObject, functionName.c_str(), &funcValue ) )
    {// Reports
        return false;
    }

    if ( funcValue.isUndefined() )
    {// Not an error: user does not handle the callback
        return true;
    }

    JS::RootedFunction func( cx, JS_ValueToFunction( cx, funcValue ) );
    if ( !func )
    {// Reports
        return false;
    }

    if ( !JS::Call( cx, globalObject, func, args, rval ) )
    {// Reports
        return false;
    }

    return true;
}

}
