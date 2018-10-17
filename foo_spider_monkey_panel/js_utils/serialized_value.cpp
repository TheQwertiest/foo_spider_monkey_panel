#include <stdafx.h>
#include "serialized_value.h"

#include <convert/native_to_js.h>
#include <convert/js_to_native.h>

namespace mozjs
{

SerializedJsValue SerializeJsValue( JSContext* cx, JS::HandleValue jsValue )
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
        serializedValue.intVal = jsValue.toInt32();
    }
    else if ( jsValue.isDouble() )
    {
        serializedValue.type = JsValueType::pt_double;
        serializedValue.doubleVal = jsValue.toDouble();
    }
    else if ( jsValue.isString() )
    {
        serializedValue.type = JsValueType::pt_string;
        JS::RootedValue rVal( cx, jsValue );
        serializedValue.strVal = convert::to_native::ToValue<pfc::string8_fast>( cx, rVal );
    }
    else
    {
        throw smp::SmpException( "Unsupported value type" ); 
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
        break;
    }
    case JsValueType::pt_int32:
    {
        jsValue.setInt32( serializedValue.intVal );
        break;
    }
    case JsValueType::pt_double:
    {
        jsValue.setDouble( serializedValue.doubleVal );
        break;
    }
    case JsValueType::pt_string:
    {
        convert::to_js::ToValue( cx, serializedValue.strVal, jsValue );
        break;
    }
    default:
    {
        return false;
    }
    }

    return true;
}

} // namespace mozjs
