#pragma once

#include <charconv>
#include <optional>
#include <string>
#include <string_view>

namespace smp::unicode
{

std::wstring ToWide( const char* ) = delete;
std::wstring ToWide_FromAcp( const char* ) = delete;
std::u8string ToU8( const wchar_t* ) = delete;
// @remark Performs double conversion (CP_ACP > Wide > CP_UTF8), use with care
std::u8string ToU8_FromAcpToWide( const wchar_t* ) = delete;

std::wstring ToWide( std::u8string_view src );
std::wstring ToWide( const pfc::string_base& src );
std::wstring ToWide_FromAcp( std::string_view src );
std::u8string ToU8( std::wstring_view src );
// @remark Performs double conversion (CP_ACP > Wide > CP_UTF8), use with care
std::u8string ToU8_FromAcpToWide( std::string_view src );

} // namespace smp::unicode
