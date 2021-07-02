#include <stdafx.h>

#include "cached_utf8_paths_hack.h"

#include <utils/md5.h>

#include <unordered_map>

namespace
{

std::unordered_map<std::string, std::filesystem::path> g_cachedPaths;

}

namespace mozjs::hack
{

std::string CacheUtf8Path( const std::filesystem::path& path )
{
    const auto u8path = path.u8string();
    const auto hash = smp::CalculateMd5( { reinterpret_cast<const uint8_t*>( u8path.c_str() ), u8path.length() } );
    g_cachedPaths.try_emplace( hash, path );
    return hash;
}

std::optional<std::filesystem::path> GetCachedUtf8Path( const std::string_view& pathId )
{
    const auto it = g_cachedPaths.find( { pathId.data(), pathId.size() } );
    if ( it == g_cachedPaths.end() )
    {
        return std::nullopt;
    }

    return it->second;
}

const std::unordered_map<std::string, std::filesystem::path>& GetAllCachedUtf8Paths()
{
    return g_cachedPaths;
}

} // namespace mozjs::hack
