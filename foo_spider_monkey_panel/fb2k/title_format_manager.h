#pragma once

namespace smp
{

class TitleFormatManager
{
public:
    ~TitleFormatManager() = default;

    static TitleFormatManager& Get();

    /// @throw qwr::QwrException
    [[nodiscard]] titleformat_object::ptr Load( const qwr::u8string& spec, const qwr::u8string& fallback ) const;
    void ClearCache();

private:
    TitleFormatManager();

private:
    mutable std::unordered_map<qwr::u8string, titleformat_object::ptr> specToTitleFormat_;
};

} // namespace smp
