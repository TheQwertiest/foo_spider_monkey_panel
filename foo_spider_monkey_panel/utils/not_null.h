///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015 Microsoft Corporation. All rights reserved.
//
// This code is licensed under the MIT License (MIT).
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
///////////////////////////////////////////////////////////////////////////////

// based on https://github.com/microsoft/GSL/blob/main/include/gsl/pointers

#pragma once

#include <algorithm>   // for forward
#include <cstddef>     // for ptrdiff_t, nullptr_t, size_t
#include <memory>      // for shared_ptr, unique_ptr
#include <type_traits> // for enable_if_t, is_convertible, is_assignable

namespace smp
{

namespace details
{
template <typename T, typename = void>
inline constexpr bool is_comparable_to_nullptr = false;

template <typename T>
    requires( std::is_convertible_v<decltype( std::declval<T>() != nullptr ), bool> )
inline constexpr bool is_comparable_to_nullptr<T> = true;

// Resolves to the more efficient of `T` or `T&`, in the context of returning a const-qualified value
// of type T.
//
// Copied from cppfront's implementation of the CppCoreGuidelines F.16 (https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rf-in)
template <typename T>
using value_or_reference_return_t = std::conditional_t<
    sizeof( T ) < 2 * sizeof( void* ) && std::is_trivially_copy_constructible<T>::value,
    T,
    T&>;

template <typename T>
using const_value_or_reference_return_t = std::conditional_t<
    sizeof( T ) < 2 * sizeof( void* ) && std::is_trivially_copy_constructible<T>::value,
    const T,
    const T&>;

} // namespace details

//
// not_null
//
// Restricts a pointer or smart pointer to only hold non-null values.
//
// Has zero size overhead over T.
//
// If T is a pointer (i.e. T == U*) then
// - allow construction from U*
// - disallow construction from nullptr_t
// - disallow default construction
// - ensure construction from null U* fails
// - allow implicit conversion to U*
//
template <class T>
class not_null
{
public:
    using element_type = T;

    static_assert( details::is_comparable_to_nullptr<T>, "T cannot be compared to nullptr." );

    template <typename U>
        requires( std::is_convertible_v<U, T> )
    constexpr not_null( U&& u )
        : ptr_( std::forward<U>( u ) )
    {
        assert( ptr_ != nullptr );
    }

    constexpr not_null( T u )
        requires( !std::is_same_v<std::nullptr_t, T> )
        : ptr_( std::move( u ) )
    {
        assert( ptr_ != nullptr );
    }

    template <typename U>
        requires( std::is_convertible_v<U, T> )
    constexpr not_null( not_null<U>&& other )
        : not_null( std::move( other.get() ) )
    {
    }

    template <typename U>
        requires( std::is_convertible_v<U, T> )
    constexpr not_null( const not_null<U>& other )
        : not_null( other.get() )
    {
    }

    not_null( const not_null& other ) = default;
    not_null& operator=( const not_null& other ) = default;

    constexpr details::const_value_or_reference_return_t<T> get() const
    {
        return ptr_;
    }

    constexpr details::value_or_reference_return_t<T> get()
    {
        return ptr_;
    }

    constexpr operator T() const
    {
        return get();
    }

    constexpr operator T()
    {
        return get();
    }

    constexpr decltype( auto ) operator->() const
    {
        return get();
    }

    constexpr decltype( auto ) operator->()
    {
        return get();
    }

    constexpr decltype( auto ) operator*() const
    {
        return *get();
    }

    constexpr decltype( auto ) operator*()
    {
        return *get();
    }

    // prevents compilation when someone attempts to assign a null pointer constant
    not_null( std::nullptr_t ) = delete;
    not_null& operator=( std::nullptr_t ) = delete;

    // unwanted operators...pointers only point to single objects!
    not_null& operator++() = delete;
    not_null& operator--() = delete;
    not_null operator++( int ) = delete;
    not_null operator--( int ) = delete;
    not_null& operator+=( std::ptrdiff_t ) = delete;
    not_null& operator-=( std::ptrdiff_t ) = delete;
    void operator[]( std::ptrdiff_t ) const = delete;

private:
    T ptr_;
};

template <class T>
auto make_not_null( T&& t ) noexcept
{
    return not_null<std::remove_cvref_t<T>>{ std::forward<T>( t ) };
}

template <class T, class... Args>
auto make_not_null_shared( Args&&... args ) noexcept
{
    return make_not_null( std::make_shared<T>( std::forward<Args>( args )... ) );
}

template <class T, class... Args>
auto make_not_null_unique( Args&&... args ) noexcept
{
    return make_not_null( std::make_unique<T>( std::forward<Args>( args )... ) );
}

template <class T, class U>
auto operator==( const not_null<T>& lhs,
                 const not_null<U>& rhs ) noexcept( noexcept( lhs.get() == rhs.get() ) )
    -> decltype( lhs.get() == rhs.get() )
{
    return lhs.get() == rhs.get();
}

template <class T, class U>
auto operator!=( const not_null<T>& lhs,
                 const not_null<U>& rhs ) noexcept( noexcept( lhs.get() != rhs.get() ) )
    -> decltype( lhs.get() != rhs.get() )
{
    return lhs.get() != rhs.get();
}

template <class T, class U>
auto operator<( const not_null<T>& lhs,
                const not_null<U>& rhs ) noexcept( noexcept( std::less<>{}( lhs.get(), rhs.get() ) ) )
    -> decltype( std::less<>{}( lhs.get(), rhs.get() ) )
{
    return std::less<>{}( lhs.get(), rhs.get() );
}

template <class T, class U>
auto operator<=( const not_null<T>& lhs,
                 const not_null<U>& rhs ) noexcept( noexcept( std::less_equal<>{}( lhs.get(), rhs.get() ) ) )
    -> decltype( std::less_equal<>{}( lhs.get(), rhs.get() ) )
{
    return std::less_equal<>{}( lhs.get(), rhs.get() );
}

template <class T, class U>
auto operator>( const not_null<T>& lhs,
                const not_null<U>& rhs ) noexcept( noexcept( std::greater<>{}( lhs.get(), rhs.get() ) ) )
    -> decltype( std::greater<>{}( lhs.get(), rhs.get() ) )
{
    return std::greater<>{}( lhs.get(), rhs.get() );
}

template <class T, class U>
auto operator>=( const not_null<T>& lhs,
                 const not_null<U>& rhs ) noexcept( noexcept( std::greater_equal<>{}( lhs.get(), rhs.get() ) ) )
    -> decltype( std::greater_equal<>{}( lhs.get(), rhs.get() ) )
{
    return std::greater_equal<>{}( lhs.get(), rhs.get() );
}

// more unwanted operators
template <class T, class U>
std::ptrdiff_t operator-( const not_null<T>&, const not_null<U>& ) = delete;
template <class T>
not_null<T> operator-( const not_null<T>&, std::ptrdiff_t ) = delete;
template <class T>
not_null<T> operator+( const not_null<T>&, std::ptrdiff_t ) = delete;
template <class T>
not_null<T> operator+( std::ptrdiff_t, const not_null<T>& ) = delete;

template <class T>
using not_null_shared = not_null<std::shared_ptr<T>>;

template <class T>
using not_null_weak = not_null<std::weak_ptr<T>>;

template <class T>
using not_null_unique = not_null<std::unique_ptr<T>>;

} // namespace smp
