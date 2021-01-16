#pragma once

#include <filesystem>

namespace smp::config
{

struct ParsedPanelSettings;

}

namespace smp::panel
{

/// @throw qwr::QwrException
[[nodiscard]] void EditScript( HWND hParent, config::ParsedPanelSettings& settings );

/// @throw qwr::QwrException
[[nodiscard]] void EditPackageScript( HWND hParent, const std::filesystem::path& script, const config::ParsedPanelSettings& settings );

} // namespace smp::panel
