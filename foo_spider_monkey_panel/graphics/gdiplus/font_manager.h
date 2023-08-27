#pragma once

namespace smp::dom
{
struct FontDescription;
}

namespace smp
{

struct GdiPlusFontData
{
    std::unique_ptr<const Gdiplus::Font> pFont;
    std::unique_ptr<const Gdiplus::FontFamily> pFontFamily;
    float ascentHeight;
    float descentHeight;
    float lineHeight;
};

class GdiPlusFontManager
{
public:
    ~GdiPlusFontManager() = default;

    static GdiPlusFontManager& Get();

    /// @throw qwr::QwrException
    [[nodiscard]] not_null_shared<const GdiPlusFontData> Load( const dom::FontDescription& fontDescription, bool isUnderlined, bool isStrikeout ) const;
    void ClearCache();

private:
    GdiPlusFontManager();

private:
    mutable std::unordered_map<std::wstring, not_null_shared<const GdiPlusFontData>> idToFont_;
};

} // namespace smp
