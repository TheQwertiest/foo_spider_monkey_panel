#pragma once

#include <config/parsed_panel_config.h>

namespace smp::config
{

// TODO: cleanup methods and their naming

[[nodiscard]] const std::filesystem::path& GetRelativePathToMainFile();

/// @throw qwr::QwrException
[[nodiscard]] std::optional<std::filesystem::path> FindPackage( const qwr::u8string& packageId );

/// @throw qwr::QwrException
[[nodiscard]] ParsedPanelSettings GetNewPackageSettings( const qwr::u8string& name );

/// @throw qwr::QwrException
[[nodiscard]] ParsedPanelSettings GetPackageSettingsFromPath( const std::filesystem::path& packagePath );

/// @throw qwr::QwrException
void FillPackageSettingsFromPath( const std::filesystem::path& packagePath, ParsedPanelSettings& settings );

/// @throw qwr::QwrException
void MaybeSavePackageData( const ParsedPanelSettings& settings );

/// @throw qwr::QwrException
[[nodiscard]] std::filesystem::path GetPackagePath( const ParsedPanelSettings& settings );

/// @throw qwr::QwrException
[[nodiscard]] std::filesystem::path GetPackageScriptsDir( const ParsedPanelSettings& settings );

/// @throw qwr::QwrException
[[nodiscard]] std::filesystem::path GetPackageAssetsDir( const ParsedPanelSettings& settings );

/// @throw qwr::QwrException
[[nodiscard]] std::filesystem::path GetPackageStorageDir( const ParsedPanelSettings& settings );

/// @throw qwr::QwrException
[[nodiscard]] std::vector<std::filesystem::path> GetPackageScriptFiles( const ParsedPanelSettings& settings );

/// @throw qwr::QwrException
[[nodiscard]] std::vector<std::filesystem::path> GetPackageFiles( const ParsedPanelSettings& settings );

} // namespace smp::config
