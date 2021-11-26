#pragma once

#include <windef.h>

#include <memory>

namespace
{

// variadic std::is_same for checking if T same as (at least) one of the listed Ts types
template <class T, class... Ts>
using is_any_same = std::disjunction<std::is_same<T,Ts>...>;

// handle-x -> "handled-x-type"
// eg: TxOF<HGDIOBJ> = "TGDIOBJ"  (aka "HGDIOBJ__" or "void");
template <typename Hx>
requires std::is_pointer<Hx>::value
using TxOF = std::remove_pointer<Hx>::type;

// "handled-x-type" -> handle-x
// eg: HGDIOBJ = HxOF<TGDIOBJ>;
template <typename Tx>
using HxOF = std::add_pointer<Tx>::type;

} // namespace

namespace std
{

// default deleter for unique_gdi_ptr<Hx> aka unique_ptr<TxOF<Hx>>
// the unique_ptr<TxOF<Hx>> semantic allows initializing a shared_ptr<TxOF<Hx>> from an unique ptr
// copying the default_delete specialized deleter, also returning a Hx from shared_ptr<TxOF<Hx>>.get()
template <class Tx>
requires is_any_same<HxOF<Tx>, HGDIOBJ, HDC, HPEN, HBRUSH, HRGN, HPALETTE, HFONT, HBITMAP>::value
struct default_delete<Tx>
{
    typedef HxOF<Tx> pointer; // return type of unique_ptr<Tx>.get()

    // specialization for HDC
    template <class Hx>
    requires is_same<Hx, HDC>::value
    void operator()( Hx obj )
    {
        DeleteDC( (HDC)obj );
    }

    // and for the rest of the gdi handles
    template <class Hx>
    requires is_any_same<Hx, HGDIOBJ, HPEN, HBRUSH, HRGN, HPALETTE, HFONT, HBITMAP>::value
    void operator()( Hx obj )
    {
        DeleteObject( (HGDIOBJ)obj );
    }
};

} // namespace std

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
requires is_any_same<Hx, HBITMAP, HBRUSH, HFONT, HPEN /*, HDC, HRGN, HPALETTE*/>::value
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
/// @return nullptr on error, otherwise creates and returns a HBITMAP
[[nodiscard]] unique_gdi_ptr<HBITMAP> CreateHBitmapFromGdiPlusBitmap( Gdiplus::Bitmap& bitmap );

// wrap gdi only drawcalls within a Gdiplus context, copy clip/transform from GDI+ -> GDI
void WrapGdiCalls( Gdiplus::Graphics* graphics, std::function<void( HDC dc )> const& GdiOnlyDrawer );

} // namespace smp::gdi

namespace
{

// shorthand to make a "RECT" from  "rectX", "rectY", "rectW", "rectH" variables
#define makeRECT( _rect_ )                          \
    RECT _rect_                                     \
    {                                               \
        static_cast<LONG>( _rect_##X ),             \
        static_cast<LONG>( _rect_##Y ),             \
        static_cast<LONG>( _rect_##X + _rect_##W ), \
        static_cast<LONG>( _rect_##Y + _rect_##H )  \
    }

// shorthand to get the width for a RECT
#define RECT_W( _rect_ ) ( _rect_.right - _rect_.left )

// shorthand to get the width for a RECT
#define RECT_H( _rect_ ) ( _rect_.bottom - _rect_.top )

}
