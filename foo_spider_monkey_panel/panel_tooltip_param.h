#pragma once

#include <string>
#include <memory>

namespace smp::panel
{

struct PanelTooltipParam
{
    HWND hTooltip = nullptr;
    SIZE tooltipSize{};

    std::wstring fontName;
    uint32_t fontSize{};
    uint32_t fontStyle{};
};

} // namespace smp::panel
