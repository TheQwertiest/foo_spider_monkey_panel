#pragma once

#include <charconv>
#include <optional>
#include <string>
#include <string_view>

namespace smp::unicode
{

std::wstring ToWide( const char* ) = delete;
std::u8string ToU8( const wchar_t* ) = delete;

std::wstring ToWide( std::u8string_view src );
std::wstring ToWide( const pfc::string_base& src );
std::u8string ToU8( std::wstring_view src );

} // namespace smp::unicode
