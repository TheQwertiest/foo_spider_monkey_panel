#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  


namespace mozjs::convert::to_native
{

template <typename ReturnType>
ReturnType ToValue( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid )
{
    static_assert(0, "Unsupported type");
    isValid = false;
    return ReturnType();
}

template <>
bool ToValue<bool>( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid );

template <>
int8_t ToValue<int8_t>( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid );

template <>
int32_t ToValue<int32_t>( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid );

template <>
uint8_t ToValue<uint8_t>( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid );

template <>
uint32_t ToValue<uint32_t>( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid );

/// @details Returns only approximate uint64_t value, use with care!
template <>
uint64_t ToValue<uint64_t>( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid );

template <>
float ToValue<float>( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid );

template <>
double ToValue<double>( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid );

template <>
std::string ToValue<std::string>( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid );

template <>
std::wstring ToValue<std::wstring>( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid );

template <>
std::nullptr_t ToValue<std::nullptr_t>( JSContext * cx, const JS::HandleValue& jsValue, bool& isValid );

}
