#pragma once

#include <windef.h>

#include <memory>

#include <utils/std_helpers.h>

namespace smp::gdi
{

/// @details Resets last status!
template <typename T>
[[nodiscard]] bool IsGdiPlusObjectValid( const T* obj )
{
    return ( obj && ( Gdiplus::Ok == obj->GetLastStatus() ) );
}

/// @details Resets last status!
template <typename T>
[[nodiscard]] bool IsGdiPlusObjectValid( const std::unique_ptr<T>& obj )
{
    return IsGdiPlusObjectValid( obj.get() );
}

template <typename Hx>
requires is_any_same<Hx, HGDIOBJ, HDC, HPEN, HBRUSH, HRGN, HPALETTE, HFONT, HBITMAP>::value
using unique_gdi_ptr = std::unique_ptr<TxOF<Hx>>;

template <typename Hx>
requires is_any_same<Hx, HBITMAP, HBRUSH, HFONT, HPEN>::value
class ObjectSelector
{
public:
    [[nodiscard]] ObjectSelector( HDC dc, Hx obj, bool delete_after = false )
        : hdc( dc )
        , old( SelectObject( dc, (HGDIOBJ)obj ) )
        , tmp( delete_after )
    {
    }

    ~ObjectSelector()
    {
        HGDIOBJ obj = SelectObject( hdc, old );

        if ( tmp )
            DeleteObject( obj );
    }

private:
    HDC hdc = nullptr;
    HGDIOBJ old = nullptr;
    bool tmp = false;
};

/// @details Does not report
/// @return nullptr - error, create HBITMAP - otherwise
[[nodiscard]] unique_gdi_ptr<HBITMAP> CreateHBitmapFromGdiPlusBitmap( Gdiplus::Bitmap& bitmap );

} // namespace smp::gdi
