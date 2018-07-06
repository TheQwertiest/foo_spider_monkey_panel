#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  


namespace mozjs::convert::to_js
{

template <typename InJsType>
bool ToValue( JSContext * cx, JS::Handle<InJsType> inValue, JS::MutableHandleValue wrappedValue )
{
    static_assert(0, "Unsupported type");
    return false;
}

template <>
bool ToValue<JSObject*>( JSContext * cx, JS::HandleObject inValue, JS::MutableHandleValue wrappedValue );

template <>
bool ToValue<JS::Value>( JSContext * cx, JS::HandleValue inValue, JS::MutableHandleValue wrappedValue );

template <typename InType>
bool ToValue( JSContext * cx, const InType& inValue, JS::MutableHandleValue wrappedValue )
{
    static_assert( 0, "Unsupported type" );
    return false;
}

template <>
bool ToValue<bool>( JSContext * cx, const bool& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool ToValue<int8_t>( JSContext * cx, const int8_t& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool ToValue<int32_t>( JSContext * cx, const int32_t& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool ToValue<uint32_t>( JSContext * cx, const uint32_t& inValue, JS::MutableHandleValue wrappedValue );

/// @details Returns only approximate uint64_t value, use with care!
template <>
bool ToValue<uint64_t>( JSContext * cx, const uint64_t& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool ToValue<double>( JSContext * cx, const double& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool ToValue<float>( JSContext * cx, const float& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool ToValue<pfc::string8_fast>( JSContext * cx, const pfc::string8_fast& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool ToValue<std::wstring_view>( JSContext * cx, const std::wstring_view& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool ToValue<std::wstring>( JSContext * cx, const std::wstring& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool ToValue<std::nullptr_t>( JSContext * cx, const std::nullptr_t& inValue, JS::MutableHandleValue wrappedValue );

template<>
bool ToValue<metadb_handle_ptr>( JSContext * cx, const metadb_handle_ptr& inValue, JS::MutableHandleValue wrappedValue );

template<>
bool ToValue<metadb_handle_list>( JSContext * cx, const metadb_handle_list& inValue, JS::MutableHandleValue wrappedValue );

template<>
bool ToValue<Gdiplus::Bitmap*>( JSContext * cx, Gdiplus::Bitmap* const& inValue, JS::MutableHandleValue wrappedValue );

}
