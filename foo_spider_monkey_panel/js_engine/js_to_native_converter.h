#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  


namespace mozjs::convert::to_native
{

template <typename ReturnType>
bool IsValue( JSContext * cx, const JS::HandleValue& jsValue )
{
    static_assert(0, "Unsupported type");
    return false;
}

template <typename ReturnType>
ReturnType ToValue( JSContext * cx, const JS::HandleValue& jsValue )
{
    static_assert(0, "Unsupported type");
    return ReturnType();
}

template <>
bool IsValue<bool>( JSContext * cx, const JS::HandleValue& jsValue );
template <>
bool ToValue<bool>( JSContext * cx, const JS::HandleValue& jsValue );

template <>
bool IsValue<int32_t>( JSContext * cx, const JS::HandleValue& jsValue );
template <>
int32_t ToValue<int32_t>( JSContext * cx, const JS::HandleValue& jsValue );

template <>
bool IsValue<uint8_t>( JSContext * cx, const JS::HandleValue& jsValue );
template <>
uint8_t ToValue<uint8_t>( JSContext * cx, const JS::HandleValue& jsValue );

template <>
bool IsValue<uint32_t>( JSContext * cx, const JS::HandleValue& jsValue );
template <>
uint32_t ToValue<uint32_t>( JSContext * cx, const JS::HandleValue& jsValue );

template <>
bool IsValue<float>( JSContext * cx, const JS::HandleValue& jsValue );
template <>
float ToValue<float>( JSContext * cx, const JS::HandleValue& jsValue );

template <>
bool IsValue<double>( JSContext * cx, const JS::HandleValue& jsValue );
template <>
double ToValue<double>( JSContext * cx, const JS::HandleValue& jsValue );

template <>
bool IsValue<std::string>( JSContext * cx, const JS::HandleValue& jsValue );
template <>
std::string ToValue<std::string>( JSContext * cx, const JS::HandleValue& jsValue );

template <>
bool IsValue<std::wstring>( JSContext * cx, const JS::HandleValue& jsValue );
template <>
std::wstring ToValue<std::wstring>( JSContext * cx, const JS::HandleValue& jsValue );

template <>
bool IsValue<std::nullptr_t>( JSContext * cx, const JS::HandleValue& jsValue );
template <>
std::nullptr_t ToValue<std::nullptr_t>( JSContext * cx, const JS::HandleValue& jsValue );
}
