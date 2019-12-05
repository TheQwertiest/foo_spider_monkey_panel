#pragma once

#include <js_utils/js_object_helper.h>

#include <optional>

#include <utils/type_traits_x.h>

namespace mozjs::convert::to_native
{

// Forward declarations

template <typename T, typename F>
void ProcessArray( JSContext* cx, JS::HandleObject jsObject, F&& workerFunc );

template <typename T, typename F>
void ProcessArray( JSContext* cx, JS::HandleValue jsValue, F&& workerFunc );

} // namespace mozjs::convert::to_native

namespace mozjs::convert::to_native::internal
{

template <class T>
inline constexpr bool IsJsSimpleConvertableImplV = std::disjunction_v<std::is_fundamental<T>, std::is_same<std::u8string, T>, std::is_same<std::wstring, T>, std::is_same<pfc::string8_fast, T>>;

template <class T>
inline constexpr bool IsJsSimpleConvertableV = IsJsSimpleConvertableImplV<std::remove_cv_t<T>>;

template <typename T>
T ToSimpleValue( JSContext* cx, const JS::HandleObject& jsObject )
{
    auto pNative = mozjs::GetInnerInstancePrivate<std::remove_pointer_t<T>>( cx, jsObject );
    smp::SmpException::ExpectTrue( pNative, "Object is not of valid type" );

    return pNative;
}

template <typename T>
T ToSimpleValue( JSContext* cx, const JS::HandleValue& jsValue )
{
    static_assert( smp::always_false_v<T>, "Unsupported type" );
}

template <>
bool ToSimpleValue<bool>( JSContext* cx, const JS::HandleValue& jsValue );

template <>
int8_t ToSimpleValue<int8_t>( JSContext* cx, const JS::HandleValue& jsValue );

template <>
int32_t ToSimpleValue<int32_t>( JSContext* cx, const JS::HandleValue& jsValue );

template <>
uint8_t ToSimpleValue<uint8_t>( JSContext* cx, const JS::HandleValue& jsValue );

template <>
uint32_t ToSimpleValue<uint32_t>( JSContext* cx, const JS::HandleValue& jsValue );

/// @details Returns only approximate int64_t value, use with care!
template <>
int64_t ToSimpleValue<int64_t>( JSContext* cx, const JS::HandleValue& jsValue );

/// @details Returns only approximate uint64_t value, use with care!
template <>
uint64_t ToSimpleValue<uint64_t>( JSContext* cx, const JS::HandleValue& jsValue );

template <>
float ToSimpleValue<float>( JSContext* cx, const JS::HandleValue& jsValue );

template <>
double ToSimpleValue<double>( JSContext* cx, const JS::HandleValue& jsValue );

template <>
std::u8string ToSimpleValue<std::u8string>( JSContext* cx, const JS::HandleValue& jsValue );

template <>
std::wstring ToSimpleValue<std::wstring>( JSContext* cx, const JS::HandleValue& jsValue );

template <>
pfc::string8_fast ToSimpleValue<pfc::string8_fast>( JSContext* cx, const JS::HandleValue& jsValue );

template <>
std::nullptr_t ToSimpleValue<std::nullptr_t>( JSContext* cx, const JS::HandleValue& jsValue );

template <typename T>
std::vector<T> ToVector( JSContext* cx, JS::HandleObject jsObject )
{
    std::vector<T> nativeValues;
    ProcessArray<T>( cx, jsObject, [&nativeValues]( T&& nativeValue ) { nativeValues.push_back( std::forward<T>( nativeValue ) ); } );

    return nativeValues;
}

template <typename T>
std::vector<T> ToVector( JSContext* cx, JS::HandleValue jsValue )
{
    JS::RootedObject jsObject( cx, jsValue.toObjectOrNull() );
    smp::SmpException::ExpectTrue( jsObject, "Value is not a JS object" );

    return ToVector<T>( cx, jsObject );
}

} // namespace mozjs::convert::to_native::internal

namespace mozjs::convert::to_native
{

template <typename T>
T ToValue( JSContext* cx, JS::HandleValue jsValue )
{
    if constexpr ( to_native::internal::IsJsSimpleConvertableV<T> )
    { // Construct and copy
        return to_native::internal::ToSimpleValue<T>( cx, jsValue );
    }
    else if constexpr ( std::is_pointer_v<T> )
    { // Extract native pointer
        // TODO: think if there is a good way to move this to convert::to_native
        if ( !jsValue.isObjectOrNull() )
        {
            throw smp::SmpException( "Value is not a JS object" );
        }

        if ( jsValue.isNull() )
        { // Not an error: null might be a valid argument
            return static_cast<T>( nullptr );
        }

        JS::RootedObject jsObject( cx, &jsValue.toObject() );
        return internal::ToSimpleValue<T>( cx, jsObject );
    }
    else if constexpr ( smp::is_specialization_of_v<T, std::vector> )
    {
        return internal::ToVector<T::value_type>( cx, jsValue );
    }
    else
    {
        static_assert( smp::always_false_v<T>, "Unsupported type" );
    }
}

template <typename T>
T ToValue( JSContext* cx, const JS::HandleString& jsString )
{
    static_assert( 0, "Unsupported type" );
}

template <>
std::u8string ToValue( JSContext* cx, const JS::HandleString& jsString );

template <>
std::wstring ToValue( JSContext* cx, const JS::HandleString& jsString );

template <>
pfc::string8_fast ToValue( JSContext* cx, const JS::HandleString& jsString );

template <typename T, typename F>
void ProcessArray( JSContext* cx, JS::HandleObject jsObject, F&& workerFunc )
{
    bool is;
    if ( !JS_IsArrayObject( cx, jsObject, &is ) )
    {
        throw smp::JsException();
    }
    smp::SmpException::ExpectTrue( is, "Object is not an array" );

    uint32_t arraySize;
    if ( !JS_GetArrayLength( cx, jsObject, &arraySize ) )
    {
        throw smp::JsException();
    }

    if ( !arraySize )
    { // small optimization
        return;
    }

    std::vector<T> nativeValues;
    JS::RootedValue arrayElement( cx );
    for ( uint32_t i = 0; i < arraySize; ++i )
    {
        if ( !JS_GetElement( cx, jsObject, i, &arrayElement ) )
        {
            throw smp::JsException();
        }

        workerFunc( ToValue<T>( cx, arrayElement ) );
    }
}

template <typename T, typename F>
void ProcessArray( JSContext* cx, JS::HandleValue jsValue, F&& workerFunc )
{
    JS::RootedObject jsObject( cx, jsValue.toObjectOrNull() );
    smp::SmpException::ExpectTrue( jsObject, "Value is not a JS object" );

    to_native::ProcessArray<T>( cx, jsObject, std::forward<F>( workerFunc ) );
}

} // namespace mozjs::convert::to_native
