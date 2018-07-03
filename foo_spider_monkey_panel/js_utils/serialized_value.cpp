#include <stdafx.h>
#include "serialized_value.h"

#include <js_engine/native_to_js_converter.h>
#include <js_engine/js_to_native_converter.h>

namespace mozjs
{

std::optional<SerializedJsValue> SerializeJsValue( JSContext* cx, JS::HandleValue jsValue )
{
    SerializedJsValue serializedValue;

    if ( jsValue.isBoolean() )
    {
        serializedValue.type = JsValueType::pt_boolean;
        serializedValue.boolVal = jsValue.toBoolean();
    }
    else if ( jsValue.isInt32() )
    {
        serializedValue.type = JsValueType::pt_int32;
        serializedValue.intVal = static_cast<uint64_t>( jsValue.toInt32() );
    }
    else if ( jsValue.isDouble() )
    {
        serializedValue.type = JsValueType::pt_double;
        serializedValue.doubleVal = jsValue.toDouble();
    }
    else if ( jsValue.isString() )
    {
        serializedValue.type = JsValueType::pt_double;
        bool isValid;
        JS::RootedValue rVal( cx, jsValue );
        serializedValue.strVal = convert::to_native::ToValue<std::wstring>( cx, rVal, isValid );
        assert( isValid );
    }
    else
    {
        assert( 0 );
        return std::nullopt;
    }

    return serializedValue;
}

bool DeserializeJsValue( JSContext* cx, const SerializedJsValue& serializedValue, JS::MutableHandleValue jsValue )
{
    switch ( serializedValue.type )
    {
    case JsValueType::pt_boolean:
    {
        jsValue.setBoolean( serializedValue.boolVal );
    }
    case JsValueType::pt_int32:
    {
        jsValue.setInt32( serializedValue.intVal );
    }
    case JsValueType::pt_double:
    {
        jsValue.setDouble( serializedValue.doubleVal );
    }
    case JsValueType::pt_string:
    {
        if ( !convert::to_js::ToValue( cx, serializedValue.strVal, jsValue ) )
        {// should not happen
            assert( 0 );
            return false;
        }

        break;
    }
    default:
    {
        return false;
    }
    }

    return true;
}

}
