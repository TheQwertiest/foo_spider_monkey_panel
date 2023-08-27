#include <stdafx.h>

#include "console.h"

#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/utils/js_object_constants.h>

#include <js/experimental/TypedData.h>
#include <qwr/final_action.h>

using namespace smp;

namespace
{

constexpr uint32_t kMaxLogDepth = 20;
constexpr uint32_t kMaxArrayElemCount = 100;

} // namespace

namespace
{

using namespace mozjs;

qwr::u8string ParseJsValue( JSContext* cx, JS::HandleValue jsValue, JS::MutableHandleObjectVector curObjects, uint32_t& logDepth, bool isParentObject );

qwr::u8string ParseJsArray( JSContext* cx, JS::HandleObject jsObject, JS::MutableHandleObjectVector curObjects, uint32_t& logDepth )
{
    qwr::u8string output;

    output += "[";

    uint32_t arraySize;
    if ( !JS::GetArrayLength( cx, jsObject, &arraySize ) )
    {
        throw JsException();
    }

    JS::RootedValue arrayElement( cx );
    const auto maxLoopCount = std::min( arraySize, kMaxArrayElemCount );
    for ( uint32_t i = 0; i < maxLoopCount; ++i )
    {
        if ( !JS_GetElement( cx, jsObject, i, &arrayElement ) )
        {
            throw JsException();
        }

        output += ParseJsValue( cx, arrayElement, curObjects, logDepth, true );
        if ( i != arraySize - 1 )
        {
            output += ", ";
        }
    }

    if ( maxLoopCount != arraySize )
    {
        output += "...";
    }

    output += "]";

    return output;
}

qwr::u8string ParseJsTypedArray( JSContext* cx, JS::HandleObject jsObject, JS::MutableHandleObjectVector curObjects, uint32_t& logDepth )
{
    qwr::u8string output;

    output += JS::InformalValueTypeName( JS::ObjectValue( *jsObject ) );

    const auto logArray = [&]( auto checker, auto getter, auto elemType ) {
        if ( !checker( jsObject.get() ) )
        {
            return false;
        }

        using T = decltype( elemType );

        size_t arraySize = 0;
        bool isShared = false;
        T* data = nullptr;
        getter( jsObject, &arraySize, &isShared, &data );

        output += fmt::format( "({}) ", arraySize );
        output += "[";
        if ( data )
        {
            const auto maxElemCount = std::min( arraySize, kMaxArrayElemCount );
            std::span<T> truncatedData( data, maxElemCount );
            output += fmt::format( "{}", fmt::join( truncatedData, ", " ) );

            if ( maxElemCount != arraySize )
            {
                output += "...";
            }
        }
        else
        {
            output += "ERROR";
        }
        output += "]";

        return true;
    };

#define SMP_LOG_ARRAY( jsType, cType )                                                    \
    if ( logArray( JS_Is##jsType##Array, js::Get##jsType##ArrayLengthAndData, cType{} ) ) \
    {                                                                                     \
        return output;                                                                    \
    }

    SMP_LOG_ARRAY( Int8, int8_t )
    SMP_LOG_ARRAY( Uint8, uint8_t )
    SMP_LOG_ARRAY( Uint8Clamped, uint8_t )
    SMP_LOG_ARRAY( Int16, int16_t )
    SMP_LOG_ARRAY( Uint16, uint16_t )
    SMP_LOG_ARRAY( Int32, int32_t )
    SMP_LOG_ARRAY( Uint32, uint32_t )
    SMP_LOG_ARRAY( Float32, float )
    SMP_LOG_ARRAY( Float64, double )

#undef SMP_LOG_ARRAY

    output += "(unknown type)";

    return output;
}

qwr::u8string ParseJsObject( JSContext* cx, JS::HandleObject jsObject, JS::MutableHandleObjectVector curObjects, uint32_t& logDepth )
{
    qwr::u8string output;

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

    JS::RootedIdVector jsVector( cx );
    if ( !js::GetPropertyKeys( cx, jsObject, 0, &jsVector ) )
    {
        throw JsException();
    }

    JS::RootedValue jsIdValue( cx );
    JS::RootedValue jsValue( cx );
    bool hasFunctions = false;
    for ( size_t i = 0, length = jsVector.length(); i < length; ++i )
    {
        const auto& jsId = jsVector[i];
        if ( !JS_GetPropertyById( cx, jsObject, jsId, &jsValue ) )
        {
            throw JsException();
        }

        if ( jsValue.isObject() && JS_ObjectIsFunction( &jsValue.toObject() ) )
        {
            hasFunctions = true;
        }
        else
        {
            jsIdValue = js::IdToValue( jsId );
            output += convert::to_native::ToValue<qwr::u8string>( cx, jsIdValue );
            output += "=";
            output += ParseJsValue( cx, jsValue, curObjects, logDepth, true );
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

qwr::u8string ParseJsValue( JSContext* cx, JS::HandleValue jsValue, JS::MutableHandleObjectVector curObjects, uint32_t& logDepth, bool isParentObject )
{
    qwr::u8string output;

    ++logDepth;
    qwr::final_action autoDecrement( [&logDepth] { --logDepth; } );

    if ( !jsValue.isObject() )
    {
        const bool showQuotes = isParentObject && jsValue.isString();

        if ( showQuotes )
        {
            output += "\"";
        }
        output += convert::to_native::ToValue<qwr::u8string>( cx, jsValue );
        if ( showQuotes )
        {
            output += "\"";
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

        if ( JS_ObjectIsFunction( jsObject ) )
        {
            output += JS::InformalValueTypeName( jsValue );
        }
        else
        {
            for ( const auto& curObject: curObjects )
            {
                if ( jsObject.get() == curObject )
                {
                    output += "<Circular>";
                    return output;
                }
            }

            if ( !curObjects.emplaceBack( jsObject ) )
            {
                throw JsException();
            }
            qwr::final_action autoPop( [&curObjects] { curObjects.popBack(); } );

            if ( JS_IsTypedArrayObject( jsObject.get() ) )
            {
                output += ParseJsTypedArray( cx, jsObject, curObjects, logDepth );
            }
            else
            {
                bool isArray;
                if ( !JS::IsArrayObject( cx, jsObject, &isArray ) )
                {
                    throw JsException();
                }

                if ( isArray )
                {
                    output += ParseJsArray( cx, jsObject, curObjects, logDepth );
                }
                else
                {
                    output += ParseJsObject( cx, jsObject, curObjects, logDepth );
                }
            }
        }
    }

    return output;
}

std::optional<qwr::u8string> ParseLogArgs( JSContext* cx, JS::CallArgs& args )
{
    if ( !args.length() )
    {
        return std::nullopt;
    }

    qwr::u8string outputString;
    JS::RootedObjectVector curObjects( cx );
    uint32_t logDepth = 0;
    for ( size_t i = 0; i < args.length(); ++i )
    {
        assert( !logDepth );
        assert( !curObjects.length() );
        outputString += ParseJsValue( cx, args[i], &curObjects, logDepth, false );
        if ( i < args.length() - 1 )
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

    console::info( output->c_str() );
    return true;
}

MJS_DEFINE_JS_FN( Log, LogImpl )

constexpr auto console_functions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "log", Log, 0, kDefaultPropsFlags ),
        JS_FS_END,
    } );
} // namespace

namespace mozjs
{

void DefineConsole( JSContext* cx, JS::HandleObject global )
{
    JS::RootedObject consoleObj( cx, JS_NewPlainObject( cx ) );
    if ( !consoleObj
         || !JS_DefineFunctions( cx, consoleObj, console_functions.data() )
         || !JS_DefineProperty( cx, global, "console", consoleObj, kDefaultPropsFlags ) )
    {
        throw JsException();
    }
}

} // namespace mozjs
