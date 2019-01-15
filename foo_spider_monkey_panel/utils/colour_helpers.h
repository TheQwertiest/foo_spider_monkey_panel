#pragma once

namespace smp::colour
{

constexpr COLORREF convert_argb_to_colorref( DWORD argb )
{
    return RGB( argb >> RED_SHIFT, argb >> GREEN_SHIFT, argb >> BLUE_SHIFT );
}

constexpr DWORD convert_colorref_to_argb( COLORREF color )
{
    // COLORREF : 0x00bbggrr
    // ARGB : 0xaarrggbb
    return ( GetRValue( color ) << RED_SHIFT )
           | ( GetGValue( color ) << GREEN_SHIFT )
           | ( GetBValue( color ) << BLUE_SHIFT )
           | 0xff000000;
}

} // namespace smp::colour
