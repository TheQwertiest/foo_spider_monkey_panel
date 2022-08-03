#pragma once

#include <filesystem>

namespace smp::config
{

class ResolvedPanelScriptSettings;

}

namespace smp::panel
{

/// @throw qwr::QwrException
void EditScript( HWND hParent, qwr::u8string& script );

/// @throw qwr::QwrException
void EditScriptFile( HWND hParent, const config::ResolvedPanelScriptSettings& settings );

/// @throw qwr::QwrException
void EditPackageScript( HWND hParent, const std::filesystem::path& script );

} // namespace smp::panel
