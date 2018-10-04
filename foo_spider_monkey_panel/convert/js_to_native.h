#pragma once

#include <optional>

namespace mozjs::convert::to_native
{

template<class T>
struct _is_convertable_v
    : std::bool_constant<std::is_fundamental_v<T>
    || std::is_same_v<pfc::string8_fast, T>
    || std::is_same_v<std::wstring, T>>
{
};

template<class T>
struct is_convertable
    : _is_convertable_v<std::remove_cv_t<T>>::type
{
};

template<class T>
inline constexpr bool is_convertable_v = is_convertable<T>::value;


template <typename T>
std::optional<T> ToValue(JSContext * cx, const JS::HandleObject& jsObject)
{
    auto pNative = GetInnerInstancePrivate<std::remove_pointer_t<T>>( cx, jsObject );
    if (!pNative)
    {
        return std::nullopt;
    }

    return pNative;
}

std::optional<std::wstring> ToValue( JSContext * cx, const JS::HandleString& jsString );

template <typename T>
std::optional<T> ToValue( JSContext * cx, const JS::HandleValue& jsValue )
{
    static_assert(0, "Unsupported type");
    isValid = false;
    return T();
}

template <>
std::optional<bool> ToValue<bool>( JSContext * cx, const JS::HandleValue& jsValue );

template <>
std::optional<int8_t> ToValue<int8_t>( JSContext * cx, const JS::HandleValue& jsValue );

template <>
std::optional<int32_t> ToValue<int32_t>( JSContext * cx, const JS::HandleValue& jsValue );

template <>
std::optional<uint8_t> ToValue<uint8_t>( JSContext * cx, const JS::HandleValue& jsValue );

template <>
std::optional<uint32_t> ToValue<uint32_t>( JSContext * cx, const JS::HandleValue& jsValue );

/// @details Returns only approximate uint64_t value, use with care!
template <>
std::optional<uint64_t> ToValue<uint64_t>( JSContext * cx, const JS::HandleValue& jsValue );

template <>
std::optional<float> ToValue<float>( JSContext * cx, const JS::HandleValue& jsValue );

template <>
std::optional<double> ToValue<double>( JSContext * cx, const JS::HandleValue& jsValue );

template <>
std::optional<pfc::string8_fast> ToValue<pfc::string8_fast>( JSContext * cx, const JS::HandleValue& jsValue );

template <>
std::optional<std::wstring> ToValue<std::wstring>( JSContext * cx, const JS::HandleValue& jsValue );

template <>
std::optional<std::nullptr_t> ToValue<std::nullptr_t>( JSContext * cx, const JS::HandleValue& jsValue );

}
