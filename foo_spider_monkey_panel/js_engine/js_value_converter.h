#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

namespace mozjs
{

template <typename InType>
bool WrapValue( JSContext * cx, const InType& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool WrapValue<bool>( JSContext * cx, const bool& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool WrapValue<int32_t>( JSContext * cx, const int32_t& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool WrapValue<double>( JSContext * cx, const double& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool WrapValue<std::nullptr_t>( JSContext * cx, const std::nullptr_t& inValue, JS::MutableHandleValue wrappedValue );

template <typename ReturnType>
bool UnwrapValue( const JS::HandleValue& jsValue, ReturnType& unwrappedValue );

template <>
bool UnwrapValue<bool>( const JS::HandleValue& jsValue, bool& unwrappedValue );

template <>
bool UnwrapValue<int32_t>( const JS::HandleValue& jsValue, int32_t& unwrappedValue );

template <>
bool UnwrapValue<double>( const JS::HandleValue& jsValue, double& unwrappedValue );

template <>
bool UnwrapValue<std::nullptr_t>( const JS::HandleValue& jsValue, std::nullptr_t& unwrappedValue );

template <int ArgArraySize, typename ArgType, typename... Args>
bool WrapArguments( JSContext * cx, JS::AutoValueArray<ArgArraySize>& wrappedArgs, uint8_t argIndex, ArgType arg, Args&&... args )
{
    return WrapValue( cx, arg, wrappedArgs[argIndex] )
        && WrapArguments( cx, wrappedArgs, argIndex + 1, args... );
}

template <int ArgArraySize>
bool WrapArguments( JSContext * cx, JS::AutoValueArray<ArgArraySize>& wrappedArgs, uint8_t argIndex )
{
    return true;
}

}
