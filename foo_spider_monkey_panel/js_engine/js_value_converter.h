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
bool NativeToJsValue( JSContext * cx, const InType& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool NativeToJsValue<bool>( JSContext * cx, const bool& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool NativeToJsValue<int32_t>( JSContext * cx, const int32_t& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool NativeToJsValue<double>( JSContext * cx, const double& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool NativeToJsValue<std::string_view>( JSContext * cx, const std::string_view& inValue, JS::MutableHandleValue wrappedValue );

template <>
bool NativeToJsValue<std::nullptr_t>( JSContext * cx, const std::nullptr_t& inValue, JS::MutableHandleValue wrappedValue );

template <typename ReturnType>
bool JsToNativeValue( JSContext * cx,  const JS::HandleValue& jsValue, ReturnType& unwrappedValue );

template <>
bool JsToNativeValue<bool>( JSContext * cx,  const JS::HandleValue& jsValue, bool& unwrappedValue );

template <>
bool JsToNativeValue<int32_t>( JSContext * cx,  const JS::HandleValue& jsValue, int32_t& unwrappedValue );

template <>
bool JsToNativeValue<float>( JSContext * cx,  const JS::HandleValue& jsValue, float& unwrappedValue );

template <>
bool JsToNativeValue<double>( JSContext * cx,  const JS::HandleValue& jsValue, double& unwrappedValue );

template <>
bool JsToNativeValue<std::string>( JSContext * cx,  const JS::HandleValue& jsValue, std::string& unwrappedValue );

template <>
bool JsToNativeValue<std::nullptr_t>( JSContext * cx,  const JS::HandleValue& jsValue, std::nullptr_t& unwrappedValue );

}
