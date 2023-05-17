#pragma once

#include <cmath>

namespace smp::dom
{

bool IsValidDouble( double value )
{
    return ( !std::isinf( value ) && !std::isnan( value ) );
}

} // namespace smp::dom
