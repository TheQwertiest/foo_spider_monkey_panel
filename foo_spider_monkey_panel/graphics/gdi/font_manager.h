#pragma once

#include <utils/lru_cache.h>

namespace smp::dom
{
struct FontDescription;
}

namespace smp
{

struct GdiFontData
{
    std::unique_ptr<CFont> pFont;
    int32_t ascentHeight;
    int32_t descentHeight;
    int32_t lineHeight;
    // TODO: fix this, probably caused by leading paddings
    int32_t magicLineHeight;
};

class GdiFontManager
{
public:
    ~GdiFontManager() = default;

    static GdiFontManager& Get();

    /// @throw qwr::QwrException
    [[nodiscard]] not_null_shared<const GdiFontData> Load( CDCHandle cdc, const dom::FontDescription& fontDescription, bool isUnderlined, bool isStrikeout ) const;
    void ClearCache();

private:
    GdiFontManager();

private:
    mutable LruCache<std::wstring, not_null_shared<const GdiFontData>> idToFont_;
    mutable std::unordered_map<std::wstring, std::weak_ptr<const GdiFontData>> idToFontWeak_;
};

} // namespace smp
