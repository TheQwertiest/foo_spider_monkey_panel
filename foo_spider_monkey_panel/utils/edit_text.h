#pragma once

#include <filesystem>

namespace smp
{

/// @throw qwr::QwrException
void EditTextFile( HWND hParent, const std::filesystem::path& file );

/// @throw qwr::QwrException
void EditText( HWND hParent, std::u8string& text );

/// @throw qwr::QwrException
void EditTextFileInternal( HWND hParent, const std::filesystem::path& file );

/// @throw qwr::QwrException
void EditTextFileExternal( const std::filesystem::path& pathToEditor, const std::filesystem::path& file );

/// @throw qwr::QwrException
void EditTextInternal( HWND hParent, std::u8string& text );

/// @throw qwr::QwrException
void EditTextExternal( HWND hParent, const std::filesystem::path& pathToEditor, std::u8string& text );

} // namespace smp
