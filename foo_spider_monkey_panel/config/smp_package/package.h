#pragma once

#include <filesystem>

namespace smp::config
{

struct SmpPackage
{
    qwr::u8string id;
    qwr::u8string name;
    qwr::u8string author;
    qwr::u8string version;
    qwr::u8string description;
    bool enableDragDrop = false;
    bool shouldGrabFocus = true;

    // convenience
    std::filesystem::path entryFile;
    std::filesystem::path packageDir;

    [[nodiscard]] std::filesystem::path GetScriptsDir() const;
    [[nodiscard]] std::filesystem::path GetAssetsDir() const;
    [[nodiscard]] std::filesystem::path GetStorageDir() const;

    /// @remark Result is sorted
    /// @throw qwr::QwrException
    [[nodiscard]] std::vector<std::filesystem::path> GetScriptFiles() const;

    /// @remark Result is sorted
    /// @throw qwr::QwrException
    [[nodiscard]] std::vector<std::filesystem::path> GetAllFiles() const;

    /// @throw qwr::QwrException
    void ValidatePackagePath() const;

    /// @throw qwr::QwrException
    void ToFile( const std::filesystem::path& packageJson ) const;

    /// @throw qwr::QwrException
    [[nodiscard]] static SmpPackage FromFile( const std::filesystem::path& packageJson );

    /// @throw qwr::QwrException
    [[nodiscard]] static SmpPackage GenerateNewPackage( const qwr::u8string& name );
};

} // namespace smp::config
