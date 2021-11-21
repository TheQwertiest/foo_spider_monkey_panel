#pragma once

#include <windef.h>

#include <memory>

namespace
{

// shorthand for: handle-x -> "handle-type-x" eg: TGDIOBJ = TxOF<HGDIOBJ>;
template <typename Hx>
requires std::is_pointer<Hx>::value
using TxOF = std::remove_pointer<Hx>::type;

// "handle-type-x" -> handle-x eg: HGDIOBJ = HxOF<TGDIOBJ>;
template <typename Tx>
using HxOF = std::add_pointer<Tx>::type;

} // namespace

namespace std
{

// utility variadic is_same for checking if T same as (at least) one of the listed Ts types
template <class T, class... Ts>
struct is_any : disjunction<is_same<T, Ts>...>
{
};

// default deleter for unique_gdi_ptr<Hx> aka unique_ptr<TxOF<Hx>>
// the unique_ptr<TxOF<Hx>> semantic allows initializing a shared_ptr<TxOF<Hx>> from an unique ptr
// copying the default_delete specialized deleter, also returning a Hx from shared_ptr<TxOF<Hx>>.get()
template <class Tx>
requires is_any<HxOF<Tx>, HGDIOBJ, HDC, HPEN, HBRUSH, HRGN, HPALETTE, HFONT, HBITMAP>::value
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
    requires is_any<Hx, HGDIOBJ, HPEN, HBRUSH, HRGN, HPALETTE, HFONT, HBITMAP>::value
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
requires std::is_any<Hx, HGDIOBJ, HDC, HPEN, HBRUSH, HRGN, HPALETTE, HFONT, HBITMAP>::value
using unique_gdi_ptr = std::unique_ptr<TxOF<Hx>>;

template <typename Hx>
requires std::is_any<Hx, HGDIOBJ, HDC, HPEN, HBRUSH, HRGN, HPALETTE, HFONT, HBITMAP>::value
[[nodiscard]] unique_gdi_ptr<Hx> CreateUniquePtr( Hx obj )
{
    return unique_gdi_ptr<Hx>( obj );
}

template <typename Hx>
requires std::is_any<Hx, HGDIOBJ, HPEN, HBRUSH, HRGN, HPALETTE, HFONT, HBITMAP>::value
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
