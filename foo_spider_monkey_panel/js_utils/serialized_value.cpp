#include <stdafx.h>

#include "serialized_value.h"

#include <convert/js_to_native.h>
#include <convert/native_to_js.h>

#include <qwr/type_traits.h>

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
    std::visit( [cx, &jsValue]( auto&& arg ) {
        using T = std::decay_t<decltype( arg )>;
        if constexpr ( std::is_same_v<T, bool> )
        {
            jsValue.setBoolean( arg );
        }
        else if constexpr ( std::is_same_v<T, int32_t> )
        {
            jsValue.setInt32( arg );
        }
        else if constexpr ( std::is_same_v<T, double> )
        {
            jsValue.setDouble( arg );
        }
        else if constexpr ( std::is_same_v<T, qwr::u8string> )
        {
            convert::to_js::ToValue( cx, arg, jsValue );
        }
        else
        {
            static_assert( qwr::always_false_v<T>, "non-exhaustive visitor!" );
        }
    },
                serializedValue );
}

} // namespace mozjs
