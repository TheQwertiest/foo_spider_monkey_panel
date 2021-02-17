#pragma once

#include <filesystem>

namespace smp
{

/// @throw qwr::QwrException
void EditTextFile( HWND hParent, const std::filesystem::path& file, bool isPanelScript, bool isModal );

/// @throw qwr::QwrException
void EditText( HWND hParent, qwr::u8string& text, bool isPanelScript );

} // namespace smp
