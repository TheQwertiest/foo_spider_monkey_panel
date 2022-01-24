#pragma once

#include <windef.h>

#include <memory>

namespace
{

// variadic std::is_same for checking if T same as (at least) one of the listed Ts types
template <class T, class... Ts>
using is_any_same = std::disjunction<std::is_same<T, Ts>...>;

// handle-x -> "handled-x-type"
// eg: TxOF<HGDIOBJ> = "TGDIOBJ"  (aka "HGDIOBJ__" or "void");
template <typename Hx>
requires std::is_pointer<Hx>::value
using TxOF = std::remove_pointer<Hx>::type;

// "handled-x-type" -> handle-x
// eg: HGDIOBJ = HxOF<TGDIOBJ>;
template <typename Tx>
using HxOF = std::add_pointer<Tx>::type;

}

namespace std
{

// default deleter for unique_gdi_ptr<Hx> aka unique_ptr<TxOF<Hx>>
// the unique_ptr<TxOF<Hx>> semantic allows initializing a shared_ptr<TxOF<Hx>> from an unique ptr
// copying the default_delete specialized deleter, also returning a Hx from shared_ptr<TxOF<Hx>>.get()
template <class Tx>
requires is_any_same<HxOF<Tx>, HDC, HGDIOBJ, HPEN, HBRUSH, HRGN, HPALETTE, HFONT, HBITMAP>::value
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
