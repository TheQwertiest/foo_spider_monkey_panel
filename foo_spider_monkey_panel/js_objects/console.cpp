#include <stdafx.h>
#include "console.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/js_object_helper.h>

// stringstream
#include <sstream>
// precision
#include <iomanip>

namespace
{

using namespace mozjs;

pfc::string8_fast ParseJsValue( JSContext* cx, JS::HandleValue jsValue );

pfc::string8_fast ParseJsArray( JSContext* cx, JS::HandleObject jsObject )
{
    pfc::string8_fast output;

    output += "[ ";

    uint32_t arraySize;
    if ( !JS_GetArrayLength( cx, jsObject, &arraySize ) )
    {
        throw smp::JsException();
    }

    JS::RootedValue arrayElement( cx );
    for ( uint32_t i = 0; i < arraySize; ++i )
    {
        if ( !JS_GetElement( cx, jsObject, i, &arrayElement ) )
        {
            throw smp::JsException();
        }

        output += ParseJsValue( cx, arrayElement );
        if ( i != arraySize - 1 )
        {
            output += ", ";
        }
    }

    output += " ]";

    return output;
}

pfc::string8_fast ParseJsObject( JSContext* cx, JS::HandleObject jsObject )
{
    pfc::string8_fast output;

    output += "{ ";

    JS::Rooted<JS::IdVector> jsVector( cx, cx );
    if ( !JS_Enumerate( cx, jsObject, &jsVector ) )
    {
        throw smp::JsException();
    }

    output += " }";

    return output;
}

pfc::string8_fast ParseJsValue( JSContext* cx, JS::HandleValue jsValue )
{
    pfc::string8_fast output;

    output += convert::to_native::ToValue<pfc::string8_fast>( cx, jsValue );
    if ( !jsValue.isObject() )
    {
        return output;
    }

    JS::RootedObject jsObject( cx, &jsValue.toObject() );

    bool is;
    if ( !JS_IsArrayObject( cx, jsObject, &is ) )
    {
        throw smp::JsException();
    }

    if ( is )
    {
        output += ParseJsArray( cx, jsObject );
    }
    else
    {
        output += " ";
        output += ParseJsObject( cx, jsObject );
    }

    return output;
}

std::optional<pfc::string8_fast> ParseLogArgs( JSContext* cx, JS::CallArgs& args )
{ // Code extracted Mozilla's "dom/console/Console.cpp"
    if ( !args.length() )
    {
        return std::nullopt;
    }

    pfc::string8_fast output;

    if ( args.length() == 1 || !args[0].isString() )
    {
        for ( uint32_t i = 0; i < args.length(); ++i )
        {
            output += convert::to_native::ToValue<pfc::string8_fast>( cx, args[i] );
            if ( i < args.length() )
            {
                output.add_char( ' ' );
            }
        }

        return output;
    }

    const pfc::string8_fast string = convert::to_native::ToValue<pfc::string8_fast>( cx, args[0] );
    output.prealloc( 2 * string.length() );

    const char* start = string.c_str();
    const char* end = string.c_str() + string.length();
    uint32_t index = 1;

    while ( start != end )
    {
        if ( *start != '%' )
        {
            output.add_char( *start );
            ++start;
            continue;
        }

        ++start;
        if ( start == end )
        {
            output.add_char( '%' );
            break;
        }

        if ( *start == '%' )
        {
            output.add_char( *start );
            ++start;
            continue;
        }

        pfc::string8_fast tmp;
        tmp.add_char( '%' );

        if ( start == end )
        {
            output.add_string_nc( tmp, tmp.length() );
            break;
        }

        char ch = *start;
        tmp.add_char( ch );
        ++start;

        switch ( ch )
        {
        case 'o':
        case 'O':
        {
            JS::RootedValue v( cx );
            if ( index < args.length() )
            {
                v = args[index++];
            }

            output += ParseJsValue( cx, v );

            break;
        }
        case 's':
        {
            if ( index < args.length() )
            {
                output += convert::to_native::ToValue<pfc::string8_fast>( cx, args[index++] );
            }
            break;
        }
        case 'd':
        case 'i':
        {
            if ( index < args.length() )
            {
                int32_t v = convert::to_native::ToValue<int32_t>( cx, args[index++] );
                output += std::to_string( v ).c_str();
            }
            break;
        }
        case 'f':
        {
            if ( index < args.length() )
            {
                double v = convert::to_native::ToValue<double>( cx, args[index++] );

                if ( std::isnan( v ) )
                {
                    output.add_string( "NaN" );
                }
                else
                {
                    // std::to_string(double) has precision of float
                    auto doubleToString = []( double dVal ) {
                        std::ostringstream out;
                        out << std::setprecision( 16 ) << dVal;
                        return out.str();
                    };

                    output += doubleToString( v ).c_str();
                }
            }
            break;
        }
        default:
        {
            output.add_string_nc( tmp, tmp.length() );
            break;
        }
        }
    }

    // The rest of the array, if unused by the format string.
    for ( ; index < args.length(); ++index )
    {
        output += convert::to_native::ToValue<pfc::string8_fast>( cx, args[index] );
        if ( index < args.length() )
        {
            output.add_char( ' ' );
        }
    }

    return output;
}

// TODO: wrap in a proper class
bool LogImpl( JSContext* cx, unsigned argc, JS::Value* vp )
{
    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );

    auto output = ParseLogArgs( cx, args );
    args.rval().setUndefined();

    if ( !output )
    {
        return true;
    }

    console::info( output.value().c_str() );
    return true;
}

MJS_DEFINE_JS_FN( Log, LogImpl )

static const JSFunctionSpec console_functions[] = {
    JS_FN( "log", Log, 0, DefaultPropsFlags() ),
    JS_FS_END
};
} // namespace

namespace mozjs
{

void DefineConsole( JSContext* cx, JS::HandleObject global )
{
    JS::RootedObject consoleObj( cx, JS_NewPlainObject( cx ) );
    if ( !consoleObj
         || !JS_DefineFunctions( cx, consoleObj, console_functions )
         || !JS_DefineProperty( cx, global, "console", consoleObj, DefaultPropsFlags() ) )
    {
        throw smp::JsException();
    }
}

} // namespace mozjs
