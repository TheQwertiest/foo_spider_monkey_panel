#pragma once

#include <filesystem>

namespace smp::utils
{

/// @throw qwr::QwrException
[[nodiscard]] std::vector<std::filesystem::path>
GetFilesRecursive( const std::filesystem::path& path );

} // namespace smp::utils
