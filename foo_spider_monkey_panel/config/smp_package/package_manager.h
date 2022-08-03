#pragma once

#include <filesystem>
#include <optional>
#include <vector>

namespace smp::config
{

class SmpPackageManager
{
public:
    [[nodiscard]] SmpPackageManager() = default;

    /// @throw qwr::QwrException
    [[nodiscard]] const std::unordered_map<qwr::u8string, std::filesystem::path>&
    GetPackages( const std::optional<std::filesystem::path>& parentDir = std::nullopt ) const;

    /// @throw qwr::QwrException
    [[nodiscard]] std::optional<std::filesystem::path>
    GetPackage( const qwr::u8string& id ) const;

    void Refresh() const;

public:
    [[nodiscard]] static std::filesystem::path GetPathForNewPackage( const qwr::u8string& id );

private:
    static std::unordered_map<qwr::u8string, std::filesystem::path> idToPackageJson_;
};

} // namespace smp::config
