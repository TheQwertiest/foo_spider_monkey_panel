#include <stdafx.h>
#include "native_to_js_invoker.h"

#include <js_objects/global_object.h>
#include <js_utils/js_error_helper.h>

#include <js/Conversions.h>


namespace mozjs
{
bool InvokeJsCallback_Impl( JSContext* cx,
                            JS::HandleObject globalObject,
                            std::string_view functionName,
                            const JS::HandleValueArray& args,
                            JS::MutableHandleValue rval )
{  
    JSAutoRequest ar( cx );
    JSAutoCompartment ac( cx, globalObject );

    JS::RootedValue funcValue( cx );
    if ( !JS_GetProperty( cx, globalObject, functionName.data(), &funcValue ) )
    {
        return false;
    }

    if ( funcValue.isUndefined() )
    {// Not an error
        return true;
    }

    JS::RootedFunction func( cx, JS_ValueToFunction( cx, funcValue ) );
    if ( !func )
    {
        return false;
    }

    AutoReportException are( cx );
    if ( !JS::Call( cx, globalObject, func, args, rval ) )
    {
        console::printf( JSP_NAME "JS::JS_Call failed\n" );
        return false;
    }

    return true;
}

}
