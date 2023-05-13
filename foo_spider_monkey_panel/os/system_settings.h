#pragma once

namespace smp::os
{

class SystemSettings
{
public:
    static SystemSettings& Instance();

    void Reinit();

public:
    uint32_t GetScrollLines() const;
    uint32_t GetScrollChars() const;
    bool IsPageScroll( bool isVertical ) const;

private:
    [[nodiscard]] SystemSettings();

private:
    // default values as per SystemParametersInfo docs
    uint32_t scrollLines_ = 3;
    uint32_t scrollChars_ = 3;
};

} // namespace smp::os
