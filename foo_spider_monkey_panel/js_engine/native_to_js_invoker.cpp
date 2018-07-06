#include <stdafx.h>
#include "native_to_js_invoker.h"

#include <js_objects/global_object.h>
#include <js_utils/js_error_helper.h>

#include <js/Conversions.h>


namespace mozjs
{
bool InvokeJsCallback_Impl( JSContext* cx,
                            JS::HandleObject globalObject,
                            pfc::string8_fast functionName,
                            const JS::HandleValueArray& args,
                            JS::MutableHandleValue rval )
{      
    AutoReportException are( cx );

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
