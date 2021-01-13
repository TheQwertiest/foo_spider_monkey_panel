#pragma once

#include <filesystem>

namespace smp::config
{

struct ParsedPanelSettings;

}

namespace smp::panel
{

/// @return true, if setting reload is required
/// @throw qwr::QwrException
[[nodiscard]] bool EditScript( HWND hParent, config::ParsedPanelSettings& settings );

/// @throw qwr::QwrException
[[nodiscard]] void EditPackageScript( HWND hParent, const std::filesystem::path& script, const config::ParsedPanelSettings& settings );

} // namespace smp::panel
