#pragma once

namespace smp::config
{

struct ParsedPanelSettings;

}

namespace smp::panel
{

/// @return true, if setting reload is required
/// @throw qwr::QwrException
[[nodiscard]] bool EditScript( HWND hParent, config::ParsedPanelSettings& settings );

} // namespace smp::panel
