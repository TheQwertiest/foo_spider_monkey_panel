#pragma once

#include <qwr/semantic_version.h>

#include <filesystem>

namespace smp::config
{

struct ModulePackage
{
    // base
    qwr::u8string name;
    qwr::u8string author;
    qwr::SemVer version;
    qwr::u8string description;
    std::optional<std::filesystem::path> mainOpt;

    // extensions
    qwr::u8string displayedName;
    bool enableDragDrop = false;
    bool shouldGrabFocus = true;

    // convenience
    std::filesystem::path packageDir;
    qwr::u8string scope;
    bool isSample = false;

    std::filesystem::path GetEntryFile() const;

    /// @throw qwr::QwrException
    void ValidatePackagePath() const;

    /// @throw qwr::QwrException
    void ToFile( const std::filesystem::path& packageJson );

    /// @throw qwr::QwrException
    [[nodiscard]] static ModulePackage FromFile( const std::filesystem::path& packageJson );
};

} // namespace smp::config
