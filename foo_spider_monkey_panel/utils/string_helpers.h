#pragma once

#include <string>
#include <string_view>
#include <optional>

namespace smp::string
{

std::wstring Trim( const std::wstring& str );
std::u8string Trim( const std::u8string& str );
pfc::string8_fast Trim( const pfc::string8_fast& str );

template <typename T>
std::vector<std::basic_string_view<T>> Split( std::basic_string_view<T> str, const std::basic_string<T>& separator )
{
    return ranges::view::split( str, separator )
           | ranges::view::transform( []( auto&& rng ) {
                 return std::basic_string_view<T>{ &*rng.begin(), static_cast<size_t>( ranges::distance( rng ) ) };
             } );
}

template <typename T>
std::vector<std::basic_string_view<T>> Split( std::basic_string_view<T> str, T separator )
{
    return Split( str, std::basic_string<T>( 1, separator ) );
}

template <typename T, typename T2>
std::optional<T> GetNumber( const std::basic_string_view<T2>& strView )
{
    T number;
    if ( auto [pos, ec] = std::from_chars( strView.data(), strView.data() + strView.size(), number );
         ec == std::errc{} )
    {
        return number;
    }
    else
    {
        return std::nullopt;
    }
}

} // namespace smp::string
