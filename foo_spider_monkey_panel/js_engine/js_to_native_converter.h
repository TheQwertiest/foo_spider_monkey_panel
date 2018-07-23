#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <optional>

namespace mozjs::convert::to_native
{

template <typename T>
std::optional<T> ToValue(JSContext * cx, const JS::HandleObject& jsObject)
{
    JS::RootedObject jsObjectLocal(cx, jsObject);
    if (js::IsProxy(jsObject))
    {
        jsObjectLocal.set(js::GetProxyTargetObject(jsObject));
    }
    T pNative = static_cast<T>(
        JS_GetInstancePrivate(cx, jsObjectLocal, &std::remove_pointer_t<T>::JsClass, nullptr)
        );
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
