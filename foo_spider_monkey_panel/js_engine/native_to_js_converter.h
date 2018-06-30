#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  


namespace mozjs::convert::to_js
{

bool ToValue( JSContext * cx, JS::HandleObject inValue, JS::MutableHandleValue wrappedValue );

template <typename InType>
bool ToValue( JSContext * cx, const InType& inValue, JS::MutableHandleValue wrappedValue )
{
    static_assert( 0, "Unsupported type" );
    return false;
}

template <>
bool ToValue<bool>( JSContext * cx, const bool& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool ToValue<int32_t>( JSContext * cx, const int32_t& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool ToValue<uint32_t>( JSContext * cx, const uint32_t& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool ToValue<uint64_t>( JSContext * cx, const uint64_t& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool ToValue<double>( JSContext * cx, const double& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool ToValue<float>( JSContext * cx, const float& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool ToValue<std::string_view>( JSContext * cx, const std::string_view& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool ToValue<std::wstring_view>( JSContext * cx, const std::wstring_view& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool ToValue<std::string>( JSContext * cx, const std::string& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool ToValue<std::wstring>( JSContext * cx, const std::wstring& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool ToValue<std::nullptr_t>( JSContext * cx, const std::nullptr_t& inValue, JS::MutableHandleValue wrappedValue );

template<>
bool ToValue<metadb_handle_ptr>( JSContext * cx, const metadb_handle_ptr& inValue, JS::MutableHandleValue wrappedValue );

}
