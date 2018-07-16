#pragma once

#include <type_traits>

namespace mozjs::convert
{

template<class T>
struct _is_primitive
    : std::bool_constant<std::is_fundamental_v<T> 
    || std::is_same_v<pfc::string8_fast, T> 
    || std::is_same_v<std::wstring, T>>
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
