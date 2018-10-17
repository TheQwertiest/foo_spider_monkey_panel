#pragma once

#include <optional>

namespace mozjs::convert::to_native
{

template <class T>
struct _is_convertable_v
    : std::bool_constant<std::is_fundamental_v<T> || std::is_same_v<pfc::string8_fast, T> || std::is_same_v<std::wstring, T>>
{
};

template <class T>
struct is_convertable
    : _is_convertable_v<std::remove_cv_t<T>>::type
{
};

template <class T>
inline constexpr bool is_convertable_v = is_convertable<T>::value;

template <typename T>
std::optional<T> ToValue( JSContext* cx, const JS::HandleObject& jsObject )
{
    auto pNative = GetInnerInstancePrivate<std::remove_pointer_t<T>>( cx, jsObject );
    if ( !pNative )
    {
        return std::nullopt;
    }

    return pNative;
}

std::optional<std::wstring> ToValue( JSContext* cx, const JS::HandleString& jsString );

template <typename T>
std::optional<T> ToValue( JSContext* cx, const JS::HandleValue& jsValue )
{
    static_assert( 0, "Unsupported type" );
    isValid = false;
    return T();
}

template <>
std::optional<bool> ToValue<bool>( JSContext* cx, const JS::HandleValue& jsValue );

template <>
std::optional<int8_t> ToValue<int8_t>( JSContext* cx, const JS::HandleValue& jsValue );

template <>
std::optional<int32_t> ToValue<int32_t>( JSContext* cx, const JS::HandleValue& jsValue );

template <>
std::optional<uint8_t> ToValue<uint8_t>( JSContext* cx, const JS::HandleValue& jsValue );

template <>
std::optional<uint32_t> ToValue<uint32_t>( JSContext* cx, const JS::HandleValue& jsValue );

/// @details Returns only approximate uint64_t value, use with care!
template <>
std::optional<uint64_t> ToValue<uint64_t>( JSContext* cx, const JS::HandleValue& jsValue );

template <>
std::optional<float> ToValue<float>( JSContext* cx, const JS::HandleValue& jsValue );

template <>
std::optional<double> ToValue<double>( JSContext* cx, const JS::HandleValue& jsValue );

template <>
std::optional<pfc::string8_fast> ToValue<pfc::string8_fast>( JSContext* cx, const JS::HandleValue& jsValue );

template <>
std::optional<std::wstring> ToValue<std::wstring>( JSContext* cx, const JS::HandleValue& jsValue );

template <>
std::optional<std::nullptr_t> ToValue<std::nullptr_t>( JSContext* cx, const JS::HandleValue& jsValue );

// TODO: think about moving to ToValue
template <typename T, typename F>
bool ProcessArray( JSContext* cx, JS::HandleObject jsObject, F&& workerFunc )
{
    bool is;
    if ( !JS_IsArrayObject( cx, jsObject, &is ) )
    { // reports
        return false;
    }

    if ( !is )
    {
        JS_ReportErrorUTF8( cx, "object is not an array" );
        return false;
    }

    uint32_t arraySize;
    if ( !JS_GetArrayLength( cx, jsObject, &arraySize ) )
    { // reports
        return false;
    }

    std::vector<T> nativeValues;
    JS::RootedValue arrayElement( cx );
    for ( uint32_t i = 0; i < arraySize; ++i )
    {
        if ( !JS_GetElement( cx, jsObject, i, &arrayElement ) )
        { // reports
            return false;
        }

        auto retVal = convert::to_native::ToValue<T>( cx, arrayElement );
        if ( !retVal )
        { // override report
            JS_ReportErrorUTF8( cx, "array[%u] is not of the required type", i );
            return false;
        }

        workerFunc( retVal.value() );
    }

    return true;
}

// TODO: think about moving to ToValue
template <typename T, typename F>
bool ProcessArray( JSContext* cx, JS::HandleValue jsValue, F&& workerFunc )
{
    JS::RootedObject jsObject( cx, jsValue.toObjectOrNull() );
    if ( !jsObject )
    {
        throw smp::SmpException( "value is not a JS object" );
    }
    return ProcessArray<T>( cx, jsObject, std::forward<F>( workerFunc ) );
}

// TODO: think about moving to ToValue
template <typename T>
std::optional<std::vector<T>> ToVector( JSContext* cx, JS::HandleObject jsObject )
{
    std::vector<T> nativeValues;
    if ( !ProcessArray<T>( cx, jsObject, [&nativeValues]( T&& nativeValue ) { nativeValues.push_back( std::forward<T>( nativeValue ) ) } ) )
    { // reports
        return std::nullopt;
    }

    return nativeValues;
}

template <typename T>
std::optional<std::vector<T>> ToVector( JSContext* cx, JS::HandleValue jsValue )
{
    std::vector<T> nativeValues;
    if ( !ProcessArray<T>( cx, jsValue, [&nativeValues]( T&& nativeValue ) { nativeValues.push_back( std::forward<T>( nativeValue ) ) } ) )
    { // reports
        return std::nullopt;
    }

    return nativeValues;
}

} // namespace mozjs::convert::to_native
