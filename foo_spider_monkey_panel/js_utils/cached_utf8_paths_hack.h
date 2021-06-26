#pragma once

#include <filesystem>

namespace mozjs::hack
{

/// @brief This is a hack, don't use it unless it's REALLY necessary
/// @throw qwr::QwrException
/// @throw smp::JsException
std::string CacheUtf8Path( const std::filesystem::path& path );
std::optional<std::filesystem::path> GetCachedUtf8Path( const std::string_view& pathId );

} // namespace mozjs::hack
