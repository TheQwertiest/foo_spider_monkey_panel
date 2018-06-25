#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  


namespace mozjs
{

bool NativeToJsValue( JSContext * cx, JS::HandleObject inValue, JS::MutableHandleValue wrappedValue );

template <typename InType>
bool NativeToJsValue( JSContext * cx, const InType& inValue, JS::MutableHandleValue wrappedValue )
{
    static_assert( 0, "Unsupported type" );
    return false;
}

template <>
bool NativeToJsValue<bool>( JSContext * cx, const bool& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool NativeToJsValue<int32_t>( JSContext * cx, const int32_t& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool NativeToJsValue<uint32_t>( JSContext * cx, const uint32_t& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool NativeToJsValue<double>( JSContext * cx, const double& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool NativeToJsValue<float>( JSContext * cx, const float& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool NativeToJsValue<std::string_view>( JSContext * cx, const std::string_view& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool NativeToJsValue<std::wstring_view>( JSContext * cx, const std::wstring_view& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool NativeToJsValue<std::wstring>( JSContext * cx, const std::wstring& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool NativeToJsValue<std::nullptr_t>( JSContext * cx, const std::nullptr_t& inValue, JS::MutableHandleValue wrappedValue );

}
