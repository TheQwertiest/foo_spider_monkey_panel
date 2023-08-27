#pragma once

#include <vector>

namespace smp
{

void ClampGdiPlusGradient( Gdiplus::PointF& gradientStart, Gdiplus::PointF& gradientEnd,
                           std::vector<float>& blendPositions,
                           const std::vector<Gdiplus::PointF>& drawLocation );

} // namespace smp
