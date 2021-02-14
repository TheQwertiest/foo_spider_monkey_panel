#pragma once

#include <filesystem>

namespace smp::config
{

struct ParsedPanelSettings;

}

namespace smp::panel
{

/// @throw qwr::QwrException
void EditScript( HWND hParent, config::ParsedPanelSettings& settings );

/// @throw qwr::QwrException
void EditPackageScript( HWND hParent, const std::filesystem::path& script, const config::ParsedPanelSettings& settings );

} // namespace smp::panel
