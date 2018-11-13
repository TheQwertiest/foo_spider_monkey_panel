#pragma once

#include <optional>

namespace mozjs::convert::to_native::internal
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

// TODO: move to a more suitable place
template <typename T>
struct is_vector : public std::false_type
{
};

template <typename T, typename A>
struct is_vector<std::vector<T, A>> : public std::true_type
{
};

template <class T>
inline constexpr bool is_vector_v = is_vector<T>::value;

template <typename T>
T ToSimpleValue( JSContext* cx, const JS::HandleObject& jsObject )
{
    auto pNative = GetInnerInstancePrivate<std::remove_pointer_t<T>>( cx, jsObject );
    if ( !pNative )
    {
        throw smp::SmpException( "Object is not of valid type" );
    }

    return pNative;
}

template <typename T>
T ToSimpleValue( JSContext* cx, const JS::HandleValue& jsValue )
{
    static_assert( 0, "Unsupported type" );
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
pfc::string8_fast ToSimpleValue<pfc::string8_fast>( JSContext* cx, const JS::HandleValue& jsValue );

template <>
std::wstring ToSimpleValue<std::wstring>( JSContext* cx, const JS::HandleValue& jsValue );

template <>
std::nullptr_t ToSimpleValue<std::nullptr_t>( JSContext* cx, const JS::HandleValue& jsValue );

template <typename T>
std::vector<T> ToVector( JSContext* cx, JS::HandleObject jsObject )
{
    std::vector<T> nativeValues;
    if ( !ProcessArray<T>( cx, jsObject, [&nativeValues]( T&& nativeValue ) { nativeValues.push_back( std::forward<T>( nativeValue ) ) } ) )
    { // reports
        return std::nullopt;
    }

    return nativeValues;
}

template <typename T>
std::vector<T> ToVector( JSContext* cx, JS::HandleValue jsValue )
{
    std::vector<T> nativeValues;
    if ( !ProcessArray<T>( cx, jsValue, [&nativeValues]( T&& nativeValue ) { nativeValues.push_back( std::forward<T>( nativeValue ) ) } ) )
    { // reports
        return std::nullopt;
    }

    return nativeValues;
}

} // namespace mozjs::convert::to_native::internal

namespace mozjs::convert::to_native
{

template <typename T>
T ToValue( JSContext* cx, JS::HandleValue jsValue )
{
    if constexpr ( to_native::internal::is_convertable_v<T> )
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
    else if constexpr ( internal::is_vector_v<T> )
    {
        return internal::ToVector<T::value_type>( cx, jsValue );
    }
    else
    {
        static_assert( 0, "Unsupported argument type" );
    }
}

std::wstring ToValue( JSContext* cx, const JS::HandleString& jsString );

template <typename T, typename F>
void ProcessArray( JSContext* cx, JS::HandleObject jsObject, F&& workerFunc )
{
    bool is;
    if ( !JS_IsArrayObject( cx, jsObject, &is ) )
    {
        throw smp::JsException();
    }

    if ( !is )
    {
        throw smp::SmpException( "Object is not an array" );
    }

    uint32_t arraySize;
    if ( !JS_GetArrayLength( cx, jsObject, &arraySize ) )
    {
        throw smp::JsException();
    }

    if ( !arraySize )
    {// small optimization
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
    if ( !jsObject )
    {
        throw smp::SmpException( "Value is not a JS object" );
    }
    to_native::ProcessArray<T>( cx, jsObject, std::forward<F>( workerFunc ) );
}

} // namespace mozjs::convert::to_native
