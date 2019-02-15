#pragma once

#include <optional>
#include <string>

namespace smp::file
{

pfc::string8_fast CleanPath( const pfc::string8_fast& path );
std::wstring CleanPathW( const std::wstring& path );
pfc::string8_fast ReadFile( const pfc::string8_fast& path, UINT codepage, bool checkFileExistense = true ) noexcept( false );
std::wstring ReadFileW( const pfc::string8_fast& path, UINT codepage, bool checkFileExistense = true ) noexcept( false );
bool WriteFile( const wchar_t* path, const pfc::string_base& content, bool write_bom = true );

UINT DetectFileCharset( const char* fileName );

std::wstring FileDialog( const std::wstring& title, 
                         bool saveFile, 
                         gsl::span<const COMDLG_FILTERSPEC> filterSpec = std::array<COMDLG_FILTERSPEC, 1>{ COMDLG_FILTERSPEC{ L"All files", L"*.*" } }, 
                         const std::wstring& defaultExtension = L"",
                         const std::wstring& defaultFilename = L"" );

} // namespace smp::file
