#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <js_objects/js_object_wrapper.h>
#include <vector>

#include <js_objects/gdi_utils.h>

namespace mozjs
{

class JsGdiFont;

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

template <typename ReturnType>
struct JsToNative
{
    static bool IsValid( JSContext * cx, const JS::HandleValue& jsValue )
    {
        static_assert( 0, "Unsupported type" );
        return false;
    }
    static ReturnType Convert( JSContext * cx, const JS::HandleValue& jsValue )
    {
        static_assert( 0, "Unsupported type" );
        return ReturnType();
    }
};

template <>
struct JsToNative<bool>
{
    static bool IsValid( JSContext * cx, const JS::HandleValue& jsValue );
    static bool Convert( JSContext * cx, const JS::HandleValue& jsValue );
};

template <>
struct JsToNative<int32_t>
{
    static bool IsValid( JSContext * cx, const JS::HandleValue& jsValue );
    static int32_t Convert( JSContext * cx, const JS::HandleValue& jsValue );
};

template <>
struct JsToNative<uint32_t>
{
    static bool IsValid( JSContext * cx, const JS::HandleValue& jsValue );
    static uint32_t Convert( JSContext * cx, const JS::HandleValue& jsValue );
};

template <>
struct JsToNative<float>
{
    static bool IsValid( JSContext * cx, const JS::HandleValue& jsValue );
    static float Convert( JSContext * cx, const JS::HandleValue& jsValue );
};

template <>
struct JsToNative<double>
{
    static bool IsValid( JSContext * cx, const JS::HandleValue& jsValue );
    static double Convert( JSContext * cx, const JS::HandleValue& jsValue );
};

template <>
struct JsToNative<std::string >
{
    static bool IsValid( JSContext * cx, const JS::HandleValue& jsValue );
    static std::string Convert( JSContext * cx, const JS::HandleValue& jsValue );
};

template <>
struct JsToNative<std::wstring >
{
    static bool IsValid( JSContext * cx, const JS::HandleValue& jsValue );
    static std::wstring Convert( JSContext * cx, const JS::HandleValue& jsValue );
};

template <>
struct JsToNative<std::nullptr_t>
{
    static bool IsValid( JSContext * cx, const JS::HandleValue& jsValue );
    static std::nullptr_t Convert( JSContext * cx, const JS::HandleValue& jsValue );
};

}
