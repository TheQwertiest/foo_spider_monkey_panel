#pragma once

#include <string>
#include <memory>

namespace smp
{

struct PanelTooltipParam
{
    HWND hTooltip = nullptr;
    SIZE tooltipSize;

    std::wstring fontName;
    float fontSize;
    uint32_t fontStyle;
};

}
