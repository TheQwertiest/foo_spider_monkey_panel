#pragma once

#include <graphics/loaded_image.h>
#include <utils/lru_cache.h>

#include <filesystem>
#include <unordered_map>

namespace smp
{

class ImageManager
{
public:
    ~ImageManager() = default;

    static ImageManager& Get();

    /// @throw qwr::QwrException
    [[nodiscard]] std::shared_ptr<const LoadedImage> GetCached( const std::wstring& uri ) const;

    /// @throw qwr::QwrException
    [[nodiscard]] static not_null_shared<const LoadedImage> Load( const std::filesystem::path& path );

    void MaybeCache( const std::wstring& uri, not_null_shared<const LoadedImage> pImage );

    void ClearCache();

private:
    ImageManager();

private:
    mutable LruCache<std::wstring, not_null_shared<const LoadedImage>> uriToImage_;
    mutable std::unordered_map<std::wstring, std::weak_ptr<const LoadedImage>> uriToImageWeak_;
};

} // namespace smp
