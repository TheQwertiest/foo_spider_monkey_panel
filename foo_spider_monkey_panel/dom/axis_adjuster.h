#pragma once

namespace smp::dom
{

template <typename T>
inline void AdjustAxis( T& axis, T& dim )
{
    if ( dim < 0 )
    {
        axis -= dim;
        dim *= -1;
    }
}

} // namespace smp::dom
