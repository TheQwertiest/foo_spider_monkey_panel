#include <stdafx.h>

#include "gradient_clamp.h"

namespace
{

auto RotateVectorCcw( Gdiplus::PointF p1, Gdiplus::PointF p2 )
{
    return Gdiplus::PointF{ -( p2.Y - p1.Y ), p2.X - p1.X };
}

/// @return -1 if left side, 1 if right side, 0 if on the line
int GetLineSide( Gdiplus::PointF p1, Gdiplus::PointF p2, Gdiplus::PointF p )
{
    auto crossProduct = ( ( p2.X - p1.X ) * ( p.Y - p1.Y ) - ( p2.Y - p1.Y ) * ( p.X - p1.X ) );
    if ( !crossProduct )
    {
        return 0;
    }
    return ( crossProduct < 0 ? 1 : -1 );
}

float CalculateLength( Gdiplus::PointF p1, Gdiplus::PointF p2 )
{
    return sqrt( ( p2.X - p1.X ) * ( p2.X - p1.X ) + ( p2.Y - p1.Y ) * ( p2.Y - p1.Y ) );
}

auto GetDistanceToFarthestLeft( Gdiplus::PointF p1, Gdiplus::PointF p2, const std::vector<Gdiplus::PointF>& ps )
{
    const auto perp = p1 + RotateVectorCcw( p1, p2 );
    const auto perpLen = CalculateLength( perp, p1 );

    float maxDist = 0;
    Gdiplus::PointF maxP = p1;
    for ( auto p: ps )
    {
        if ( GetLineSide( p1, perp, p ) >= 0 )
        {
            continue;
        }

        const auto dist =
            abs( ( perp.X - p1.X ) * ( p1.Y - p.Y ) - ( p1.X - p.X ) * ( perp.Y - p1.Y ) )
            / perpLen;
        if ( dist > maxDist )
        {
            maxP = p;
            maxDist = dist;
        }
    }

    return maxDist;
}

auto GetDistanceToFarthestRight( Gdiplus::PointF p1, Gdiplus::PointF p2, const std::vector<Gdiplus::PointF>& ps )
{
    const auto perp = p2 + RotateVectorCcw( p1, p2 );
    const auto perpLen = CalculateLength( perp, p2 );

    float maxDist = 0;
    Gdiplus::PointF maxP = p1;
    for ( auto p: ps )
    {
        if ( GetLineSide( p2, perp, p ) <= 0 )
        {
            continue;
        }

        auto dist =
            abs( ( perp.X - p2.X ) * ( p2.Y - p.Y ) - ( p2.X - p.X ) * ( perp.Y - p2.Y ) )
            / perpLen;
        if ( dist > maxDist )
        {
            maxP = p;
            maxDist = dist;
        }
    }

    return maxDist;
}

void ClampPositions( Gdiplus::PointF& gradientStart, Gdiplus::PointF& gradientEnd, std::vector<float>& blendPositions, float leftShift, float rightShift )
{
    auto curLen = CalculateLength( gradientStart, gradientEnd );
    auto newLen = curLen + leftShift;
    if ( leftShift )
    {
        for ( auto& f: blendPositions )
        {
            f = ( f * curLen + leftShift ) / newLen;
        }

        const auto lenRatio = leftShift / curLen;
        gradientStart.X -= ( gradientEnd.X - gradientStart.X ) * lenRatio;
        gradientStart.Y -= ( gradientEnd.Y - gradientStart.Y ) * lenRatio;
    }

    curLen = newLen;
    newLen = curLen + rightShift;
    if ( rightShift )
    {
        for ( auto& f: blendPositions )
        {
            f = f * curLen / newLen;
        }

        const auto lenRatio = rightShift / curLen;
        gradientEnd.X += ( gradientEnd.X - gradientStart.X ) * lenRatio;
        gradientEnd.Y += ( gradientEnd.Y - gradientStart.Y ) * lenRatio;
    }
}

} // namespace

namespace smp::utils
{

void ClampGradient( Gdiplus::PointF& gradientStart, Gdiplus::PointF& gradientEnd,
                    std::vector<float>& blendPositions,
                    const std::vector<Gdiplus::PointF>& drawLocation )
{
    const auto leftShift = GetDistanceToFarthestLeft( gradientStart, gradientEnd, drawLocation );
    const auto rightShift = GetDistanceToFarthestRight( gradientStart, gradientEnd, drawLocation );
    ClampPositions( gradientStart, gradientEnd, blendPositions, leftShift, rightShift );
}

} // namespace smp::utils
