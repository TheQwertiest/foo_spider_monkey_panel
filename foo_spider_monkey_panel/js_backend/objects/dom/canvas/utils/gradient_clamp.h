#pragma once

#include <vector>

namespace smp::utils
{

// TODO: move to gdi utils
void ClampGradient( Gdiplus::PointF& gradientStart, Gdiplus::PointF& gradientEnd,
                    std::vector<float>& blendPositions,
                    const std::vector<Gdiplus::PointF>& drawLocation );

} // namespace smp::utils
