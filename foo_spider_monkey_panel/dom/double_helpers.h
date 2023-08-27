#pragma once

#include <cmath>

namespace smp::dom
{

inline bool IsValidDouble( double value )
{
    return ( !std::isinf( value ) && !std::isnan( value ) );
}

} // namespace smp::dom
