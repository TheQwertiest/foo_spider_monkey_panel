#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

namespace mozjs
{

template <typename InType>
void WrapValue( const InType& inValue, JS::MutableHandleValue wrappedValue );

template <>
void WrapValue<bool>( const bool& inValue, JS::MutableHandleValue wrappedValue );

template <>
void WrapValue<int32_t>( const int32_t& inValue, JS::MutableHandleValue wrappedValue );

template <>
void WrapValue<double>( const double& inValue, JS::MutableHandleValue wrappedValue );

template <>
void WrapValue<std::nullptr_t>( const std::nullptr_t& inValue, JS::MutableHandleValue wrappedValue );

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
void WrapArguments( JS::AutoValueArray<ArgArraySize>& wrappedArgs, uint8_t argIndex, ArgType arg, Args&&... args )
{
    WrapValue( arg, wrappedArgs[argIndex] );
    WrapArguments( wrappedArgs, argIndex + 1, args... );
}

template <int ArgArraySize>
void WrapArguments( JS::AutoValueArray<ArgArraySize>& wrappedArgs, uint8_t argIndex )
{
}

}
