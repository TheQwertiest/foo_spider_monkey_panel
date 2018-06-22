#include <stdafx.h>

#include "js_value_converter.h"


namespace mozjs
{

bool WrapValue( JSContext * cx, JS::HandleObject inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setObjectOrNull( inValue );
    return true;
}

template <>
bool WrapValue<bool>( JSContext *, const bool& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setBoolean( inValue );
    return true;
}

template <>
bool WrapValue<int32_t>( JSContext *, const int32_t& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setInt32( inValue );
    return true;
}

template <>
bool WrapValue<uint32_t>( JSContext *, const uint32_t& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setNumber( inValue );
    return true;
}

template <>
bool WrapValue<double>( JSContext *, const double& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setNumber( inValue );
    return true;
}

template <>
bool WrapValue<std::string_view>( JSContext * cx, const std::string_view& inValue, JS::MutableHandleValue wrappedValue )
{
    JSString* jsString = JS_NewStringCopyZ( cx, inValue.data() );
    if ( !jsString )
    {
        return false;
    }

    wrappedValue.setString( jsString );
    return true;
}

template <>
bool WrapValue<std::nullptr_t>( JSContext *, const std::nullptr_t& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setUndefined();
    return true;
}

template <>
bool UnwrapValue<bool>( const JS::HandleValue& jsValue, bool& unwrappedValue )
{
    if ( !jsValue.isBoolean() )
    {
        return false;
    }

    unwrappedValue = jsValue.toBoolean();
    return true;
}

template <>
bool UnwrapValue<int32_t>( const JS::HandleValue& jsValue, int32_t& unwrappedValue )
{
    if ( !jsValue.isInt32() )
    {
        return false;
    }

    unwrappedValue = jsValue.toInt32();
    return true;
}

template <>
bool UnwrapValue<uint32_t>( const JS::HandleValue& jsValue, uint32_t& unwrappedValue )
{
    if ( !jsValue.isNumber() )
    {
        return false;
    }

    unwrappedValue = static_cast<uint32_t>( jsValue.toNumber() );
    return true;
}

template <>
bool UnwrapValue<float>( const JS::HandleValue& jsValue, float& unwrappedValue )
{
    if ( !jsValue.isNumber() )
    {
        return false;
    }

    unwrappedValue = static_cast<float>( jsValue.toNumber() );
    return true;
}

template <>
bool UnwrapValue<double>( const JS::HandleValue& jsValue, double& unwrappedValue )
{
    if ( !jsValue.isNumber() )
    {
        return false;
    }

    unwrappedValue = jsValue.toNumber();
    return true;
}

template <>
bool UnwrapValue<std::nullptr_t>( const JS::HandleValue& jsValue, std::nullptr_t& unwrappedValue )
{
    return true;
}

}
