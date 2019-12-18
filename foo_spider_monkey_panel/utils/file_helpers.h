#pragma once

#include <nonstd/span.hpp>

#include <optional>
#include <string>

namespace smp::file
{

/// @throw smp::SmpException
std::u8string ReadFile( const std::u8string& path, UINT codepage, bool checkFileExistense = true );

/// @throw smp::SmpException
std::wstring ReadFileW( const std::u8string& path, UINT codepage, bool checkFileExistense = true );

bool WriteFile( const wchar_t* path, const std::u8string& content, bool write_bom = true );

UINT DetectFileCharset( const std::u8string& path );

std::wstring FileDialog( const std::wstring& title,
                         bool saveFile,
                         nonstd::span<const COMDLG_FILTERSPEC> filterSpec = std::array<COMDLG_FILTERSPEC, 1>{ COMDLG_FILTERSPEC{ L"All files", L"*.*" } },
                         const std::wstring& defaultExtension = L"",
                         const std::wstring& defaultFilename = L"" );

} // namespace smp::file
