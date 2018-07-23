#pragma once

#include <type_traits>

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

}
