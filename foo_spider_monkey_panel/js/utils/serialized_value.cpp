#include <stdafx.h>

#include "serialized_value.h"

#include <convert/js_to_native.h>
#include <convert/native_to_js.h>

#include <qwr/visitor.h>

namespace mozjs
{

SerializedJsValue SerializeJsValue( JSContext* cx, JS::HandleValue jsValue )
{
    SerializedJsValue serializedValue;

    if ( jsValue.isBoolean() )
    {
        serializedValue = jsValue.toBoolean();
    }
    else if ( jsValue.isInt32() )
    {
        serializedValue = jsValue.toInt32();
    }
    else if ( jsValue.isDouble() )
    {
        serializedValue = jsValue.toDouble();
    }
    else if ( jsValue.isString() )
    {
        JS::RootedValue rVal( cx, jsValue );
        serializedValue = convert::to_native::ToValue<qwr::u8string>( cx, rVal );
    }
    else
    {
        throw qwr::QwrException( "Unsupported value type" );
    }

    return serializedValue;
}

void DeserializeJsValue( JSContext* cx, const SerializedJsValue& serializedValue, JS::MutableHandleValue jsValue )
{
    std::visit(
        qwr::Visitor{
            [cx, &jsValue]( bool arg ) { jsValue.setBoolean( arg ); },
            [cx, &jsValue]( int32_t arg ) { jsValue.setInt32( arg ); },
            [cx, &jsValue]( double arg ) { jsValue.setDouble( arg ); },
            [cx, &jsValue]( const qwr::u8string& arg ) { convert::to_js::ToValue( cx, arg, jsValue ); } },
        serializedValue );
}

} // namespace mozjs
