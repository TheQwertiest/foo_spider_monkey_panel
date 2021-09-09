#pragma once

namespace smp::colour
{

constexpr [[nodiscard]] COLORREF ArgbToColorref( DWORD argb )
{
    return RGB( argb >> RED_SHIFT, argb >> GREEN_SHIFT, argb >> BLUE_SHIFT );
}

constexpr [[nodiscard]] DWORD ColorrefToArgb( COLORREF color )
{
    // COLORREF : 0x00bbggrr
    // ARGB : 0xaarrggbb
    return ( GetRValue( color ) << RED_SHIFT )
           | ( GetGValue( color ) << GREEN_SHIFT )
           | ( GetBValue( color ) << BLUE_SHIFT )
           | 0xff000000;
}

} // namespace smp::colour
