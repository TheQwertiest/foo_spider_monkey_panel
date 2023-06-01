#pragma once

#include <vector>

namespace smp::graphics
{

void ClampGradient( Gdiplus::PointF& gradientStart, Gdiplus::PointF& gradientEnd,
                    std::vector<float>& blendPositions,
                    const std::vector<Gdiplus::PointF>& drawLocation );

} // namespace smp::graphics
