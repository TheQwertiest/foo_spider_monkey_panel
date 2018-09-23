#include <stdafx.h>
#include "console.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/js_object_helper.h>

namespace
{

using namespace mozjs;

// TODO: wrap in a proper class
// TODO: add printf-like formating as required by W3C
bool LogImpl( JSContext* cx, unsigned argc, JS::Value* vp )
{
    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );

    pfc::string8_fast outputString;

    for ( unsigned i = 0; i < args.length(); i++ )
    {
        auto retVal = convert::to_native::ToValue<pfc::string8_fast>( cx, args[i] );
        if ( !retVal )
        {
            return false;
        }

        outputString += retVal.value();
        if ( i < args.length() )
        {
            outputString.add_char( ' ' );
        }
    }

    args.rval().setUndefined();

    console::info( outputString.c_str() );
    return true;
}

MJS_WRAP_JS_TO_NATIVE_FN( Log, LogImpl )

static const JSFunctionSpec console_functions[] = {
    JS_FN( "log", Log, 0, DefaultPropsFlags() ),
    JS_FS_END
};
} // namespace

namespace mozjs
{

bool DefineConsole( JSContext* cx, JS::HandleObject global )
{
    JS::RootedObject consoleObj( cx, JS_NewPlainObject( cx ) );
    return consoleObj
           && JS_DefineFunctions( cx, consoleObj, console_functions )
           && JS_DefineProperty( cx, global, "console", consoleObj, DefaultPropsFlags() );
}

} // namespace mozjs
