#include <stdafx.h>

#include "js_value_converter.h"


namespace mozjs
{

template <>
void WrapValue<bool>( const bool& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setBoolean( inValue );
}

template <>
void WrapValue<int32_t>( const int32_t& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setInt32( inValue );
}

template <>
void WrapValue<uint32_t>( const uint32_t& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setNumber( inValue );
}

template <>
void WrapValue<double>( const double& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setNumber( inValue );
}

template <>
void WrapValue<std::nullptr_t>( const std::nullptr_t& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setUndefined();
}

template <>
bool UnwrapValue<bool>( const JS::HandleValue& jsValue, bool& unwrappedValue )
{
    return jsValue.isBoolean() ? jsValue.toBoolean() : false;
}

template <>
bool UnwrapValue<int32_t>( const JS::HandleValue& jsValue, int32_t& unwrappedValue )
{
    return jsValue.isInt32() ? jsValue.toInt32() : false;
}

template <>
bool UnwrapValue<uint32_t>( const JS::HandleValue& jsValue, uint32_t& unwrappedValue )
{
    return jsValue.isNumber() ? jsValue.toNumber() : false;
}

template <>
bool UnwrapValue<double>( const JS::HandleValue& jsValue, double& unwrappedValue )
{
    return jsValue.isNumber() ? jsValue.toNumber() : false;
}

template <>
bool UnwrapValue<std::nullptr_t>( const JS::HandleValue& jsValue, std::nullptr_t& unwrappedValue )
{
    return true;
}

}
