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

#define SMP_GDI_DEFINE_SMART_HANDLES( handle, Hx )                          \
    using unique_##handle = unique_gdi_ptr<Hx>;                             \
    using shared_##handle = std::shared_ptr<unique_##handle::element_type>; \
    using   weak_##handle = shared_##handle::weak_type;

SMP_GDI_DEFINE_SMART_HANDLES( hfont, HFONT );
SMP_GDI_DEFINE_SMART_HANDLES( hbitmap, HBITMAP );


template <typename Hx>
concept Selectable = is_any_same<Hx, HBITMAP, HBRUSH, HFONT, HPEN>::value;

template <typename Hx>
requires Selectable<Hx>
class ObjectSelector
{
    const HDC hdc = {};
    const HGDIOBJ old = {};

public:
    [[nodiscard]] ObjectSelector( const HDC dc, const Hx obj ) : hdc( dc ), old( SelectObject( hdc, (HGDIOBJ)obj ) ) { }
    ~ObjectSelector() { SelectObject( hdc, old ); }
};

template <typename Hx>
requires Selectable<Hx>
class TempObjectSelector
{
    const HDC hdc = {};
    const HGDIOBJ old = {};

public:
    [[nodiscard]] TempObjectSelector( const HDC dc, Hx obj ) : hdc( dc ), old( SelectObject( hdc, (HGDIOBJ)obj ) ) { }
    ~TempObjectSelector() { DeleteObject( SelectObject( hdc, old ) ); }
};


/// @details Does not report
/// @return nullptr - error, create HBITMAP - otherwise
[[nodiscard]] unique_hbitmap CreateHBitmapFromGdiPlusBitmap( Gdiplus::Bitmap& bitmap );

// helper fn to make LOGFONTW-s using unified params
void MakeLogfontW( LOGFONTW& logfontOut,
                   const std::wstring& fontName = L"", const int32_t fontSize = 0, const uint32_t fontStyle = 0 );

} // namespace smp::gdi
