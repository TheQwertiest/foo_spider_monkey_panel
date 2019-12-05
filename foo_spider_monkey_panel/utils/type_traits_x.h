#pragma once

#include <type_traits>

namespace smp
{

template <class... Ts>
inline constexpr bool always_false_v = false;

namespace detail
{
template <template <class...> class Expr, class SFINAE, class... Args>
inline constexpr bool is_detected = false;

template <template <class...> class Expr, class... Args>
inline constexpr bool is_detected<Expr, std::void_t<Expr<Args...>>, Args...> = true;
} // namespace detail

template <template <class...> class Expr, class... Args>
inline constexpr bool is_detected_v = detail::is_detected<Expr, void, Args...>;

template <typename T, template <typename...> class TemplateT>
inline constexpr bool is_specialization_of_v = false;

template <template <typename...> class TemplateT, typename... Args>
inline constexpr bool is_specialization_of_v<TemplateT<Args...>, TemplateT> = true;

} // namespace smp
