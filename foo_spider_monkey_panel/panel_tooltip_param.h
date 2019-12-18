#pragma once

#include <memory>
#include <string>

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
