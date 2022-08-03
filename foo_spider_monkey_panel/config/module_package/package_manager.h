#pragma once

#include <filesystem>
#include <optional>
#include <vector>

namespace smp::config
{

class ModulePackageManager
{
public:
    [[nodiscard]] ModulePackageManager() = default;

    /// @throw qwr::QwrException
    [[nodiscard]] std::vector<std::filesystem::path>
    GetPackages( const std::optional<std::filesystem::path>& parentDir = std::nullopt ) const;

    /// @throw qwr::QwrException
    [[nodiscard]] std::optional<std::filesystem::path>
    GetPackage( const qwr::u8string& name ) const;

    void Refresh();

public:
    [[nodiscard]] static std::filesystem::path GetPathForNewPackage( const qwr::u8string& name );
    [[nodiscard]] static bool IsSamplePackage( const std::filesystem::path& packageJson );

private:
    // TODO: add caching
};

} // namespace smp::config
