#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  


namespace mozjs
{

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
struct JsToNative<uint8_t>
{
    static bool IsValid( JSContext * cx, const JS::HandleValue& jsValue );
    static uint8_t Convert( JSContext * cx, const JS::HandleValue& jsValue );
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
