#pragma once

#include <optional>
#include <string>

namespace smp::file
{

pfc::string8_fast CleanPath( const pfc::string8_fast& path );
std::wstring CleanPathW( const std::wstring& path );
pfc::string8_fast ReadFile( const pfc::string8_fast& path, UINT codepage ) noexcept( false );
std::wstring ReadFileW( const pfc::string8_fast& path, UINT codepage ) noexcept( false );
bool WriteFile( const char* path, const pfc::string_base& content, bool write_bom = true );

UINT DetectFileCharset( const char* fileName );

} // namespace smp::file
