#pragma once

namespace smp
{

inline bool IsEpsilonEqual( float a, float b )
{
    return std::abs( a - b ) <= 1e-5;
}

inline bool IsEpsilonLess( float a, float b )
{
    return !IsEpsilonEqual( a, b ) && a < b;
}

inline bool IsEpsilonGreater( float a, float b )
{
    return !IsEpsilonEqual( a, b ) && a > b;
}

} // namespace smp
