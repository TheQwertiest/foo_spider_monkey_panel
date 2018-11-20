#include <stdafx.h>
#include "console.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/js_object_helper.h>
#include <utils/scope_helpers.h>


using namespace smp;

namespace
{

constexpr uint32_t kMaxLogDepth = 20;

} // namespace

namespace
{

using namespace mozjs;

pfc::string8_fast ParseJsValue( JSContext* cx, JS::HandleValue jsValue, JS::AutoObjectVector& curObjects, uint32_t& logDepth );

pfc::string8_fast ParseJsArray( JSContext* cx, JS::HandleObject jsObject, JS::AutoObjectVector& curObjects, uint32_t& logDepth )
{
    pfc::string8_fast output;

    output += "[";

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

        output += ParseJsValue( cx, arrayElement, curObjects, logDepth );
        if ( i != arraySize - 1 )
        {
            output += ", ";
        }
    }

    output += "]";

    return output;
}

pfc::string8_fast ParseJsObject( JSContext* cx, JS::HandleObject jsObject, JS::AutoObjectVector& curObjects, uint32_t& logDepth )
{
    pfc::string8_fast output;

    {
        JS::RootedObject jsUnwrappedObject( cx, jsObject );
        if ( js::IsWrapper( jsObject ) )
        {
            jsUnwrappedObject = js::UncheckedUnwrap( jsObject );
        }
        if ( js::IsProxy( jsUnwrappedObject ) && js::GetProxyHandler( jsUnwrappedObject )->family() == GetSmpProxyFamily() )
        {
            jsUnwrappedObject = js::GetProxyTargetObject( jsUnwrappedObject );
        }

        output += JS::InformalValueTypeName( JS::ObjectValue( *jsUnwrappedObject ) );
    }
    output += " {";

    JS::AutoIdVector jsVector( cx );
    if ( !js::GetPropertyKeys( cx, jsObject, 0, &jsVector ) )
    {
        throw smp::JsException();
    }

    JS::RootedValue jsIdValue( cx );
    JS::RootedValue jsValue( cx );
    bool hasFunctions = false;
    for ( size_t i = 0, length = jsVector.length(); i < length; ++i )
    {
        auto& jsId = jsVector[i];
        if ( !JS_GetPropertyById( cx, jsObject, jsId, &jsValue ) )
        {
            throw smp::JsException();
        }

        if ( jsValue.isObject() && JS_ObjectIsFunction( cx, &jsValue.toObject() ) )
        {
            hasFunctions = true;
        }
        else
        {
            jsIdValue = js::IdToValue( jsId );
            output += convert::to_native::ToValue<pfc::string8_fast>( cx, jsIdValue );
            output += "=";
            output += ParseJsValue( cx, jsValue, curObjects, logDepth );
            if ( i != length - 1 || hasFunctions )
            {
                output += ", ";
            }
        }
    }

    if ( hasFunctions )
    {
        output += "...";
    }

    output += "}";

    return output;
}

pfc::string8_fast ParseJsValue( JSContext* cx, JS::HandleValue jsValue, JS::AutoObjectVector& curObjects, uint32_t& logDepth )
{
    pfc::string8_fast output;

    ++logDepth;
    utils::final_action autoDecrement( [&logDepth] { --logDepth; } );

    if ( !jsValue.isObject() )
    {
        const bool isString = jsValue.isString();

        if ( isString )
        {
            output += "'";
        }
        output += convert::to_native::ToValue<pfc::string8_fast>( cx, jsValue );
        if ( isString )
        {
            output += "'";
        }
    }
    else
    {
        if ( logDepth > kMaxLogDepth )
        { // Don't parse object, if we reached the depth limit
            output += JS::InformalValueTypeName( jsValue );
            return output;
        }

        JS::RootedObject jsObject( cx, &jsValue.toObject() );

        if ( JS_ObjectIsFunction( cx, jsObject ) )
        {
            output += JS::InformalValueTypeName( jsValue );
        }
        else
        {
            for ( const auto& curObject : curObjects )
            {
                if ( jsObject.get() == curObject )
                {
                    output += "<Circular>";
                    return output;
                }
            }

            curObjects.emplaceBack( jsObject );
            utils::final_action autoPop( [&curObjects] { curObjects.popBack(); } );

            bool is;
            if ( !JS_IsArrayObject( cx, jsObject, &is ) )
            {
                throw smp::JsException();
            }

            if ( is )
            {
                output += ParseJsArray( cx, jsObject, curObjects, logDepth );
            }
            else
            {
                output += ParseJsObject( cx, jsObject, curObjects, logDepth );
            }
        }
    }

    return output;
}

std::optional<pfc::string8_fast> ParseLogArgs( JSContext* cx, JS::CallArgs& args )
{
    if ( !args.length() )
    {
        return std::nullopt;
    }

    pfc::string8_fast outputString;
    JS::AutoObjectVector curObjects( cx );
    uint32_t logDepth = 0;
    for ( unsigned i = 0; i < args.length(); i++ )
    {
        assert( !logDepth );
        assert( !curObjects.length() );
        outputString += ParseJsValue( cx, args[i], curObjects, logDepth );
        if ( i < args.length() )
        {
            outputString += " ";
        }
    }

    return outputString;
}

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
