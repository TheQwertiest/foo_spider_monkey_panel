#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <charconv>

namespace smp::unicode
{

std::wstring ToWide( std::u8string_view src );
std::wstring ToWide( const pfc::string_base& src );
std::u8string ToU8( std::wstring_view src );

} // namespace smp::string
