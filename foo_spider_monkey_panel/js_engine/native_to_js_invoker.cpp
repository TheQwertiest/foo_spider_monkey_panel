#include <stdafx.h>

#include "native_to_js_invoker.h"

#include <js/Conversions.h>

namespace mozjs::internal
{
bool InvokeJsCallback_Impl( JSContext* cx,
                            JS::HandleObject globalObject,
                            JS::HandleValue functionValue,
                            const JS::HandleValueArray& args,
                            JS::MutableHandleValue rval )
{
    JS::RootedFunction func( cx, JS_ValueToFunction( cx, functionValue ) );
    if ( !func )
    { // Reports
        return false;
    }

    return JS::Call( cx, globalObject, func, args, rval ); // reports
}

} // namespace mozjs::internal
