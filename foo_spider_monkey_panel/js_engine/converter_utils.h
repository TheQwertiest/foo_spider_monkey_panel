#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <type_traits>

namespace mozjs::convert
{

template<class T>
struct _is_primitive
    : std::false_type
{
};

template<>
struct _is_primitive<bool>
    : std::true_type
{
};

template<>
struct _is_primitive<int32_t>
    : std::true_type
{
};

template<>
struct _is_primitive<uint8_t>
    : std::true_type
{
};

template<>
struct _is_primitive<uint32_t>
    : std::true_type
{
};

template<>
struct _is_primitive<double>
    : std::true_type
{
};

template<>
struct _is_primitive<float>
    : std::true_type
{
};

template<>
struct _is_primitive<std::string>
    : std::true_type
{
};

template<>
struct _is_primitive<std::wstring>
    : std::true_type
{
};

template<>
struct _is_primitive<std::nullptr_t>
    : std::true_type
{
};

template<class T>
struct is_primitive
    : _is_primitive<std::remove_cv_t<T>>::type
{
};

template<class T>
inline constexpr bool is_primitive_v = is_primitive<T>::value;

}
