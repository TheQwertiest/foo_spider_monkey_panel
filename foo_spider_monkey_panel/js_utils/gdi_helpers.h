#pragma once

#include <js_utils/scope_helper.h>

#include <windef.h>

#include <memory>

namespace mozjs::gdi
{

template<class T>
bool IsGdiPlusObjectValid( T* obj )
{
    return ( obj && ( Gdiplus::Ok == obj->GetLastStatus() ) );
}

// TODO: replace with template unique_gdi_ptr

using unique_bitmap_ptr = scope::unique_ptr<std::remove_pointer_t<HBITMAP>>;
using unique_dc_ptr = scope::unique_ptr<std::remove_pointer_t<HDC>>;
using unique_font_ptr = scope::unique_ptr<std::remove_pointer_t<HFONT>>;
using unique_brush_ptr = scope::unique_ptr<std::remove_pointer_t<HBRUSH>>;

template<typename T>
scope::unique_ptr<T> CreateUniquePtr( T* pObject )
{
    if constexpr ( std::is_same_v<T*, HBITMAP> )
    {
        return unique_bitmap_ptr( pObject, []( auto pObject ) { DeleteObject( pObject ); } );
    }
    else if constexpr ( std::is_same_v<T*, HDC> )
    {
        return unique_dc_ptr( pObject, []( auto pObject ) { DeleteDC( pObject ); } );
    }
    else if constexpr ( std::is_same_v<T*, HFONT> )
    {
        return unique_font_ptr( pObject, []( auto pObject ) { DeleteObject( pObject ); } );
    }
    else if constexpr ( std::is_same_v<T*, HBRUSH> )
    {
        return unique_brush_ptr( pObject, []( auto pObject ) { DeleteObject( pObject ); } );
    }
    else
    {
        static_assert( 0, "Unsupported type" );
    }
}

/// @details Does not report
/// @return nullptr - error, create HBITMAP - otherwise
unique_bitmap_ptr CreateHBitmapFromGdiPlusBitmap( Gdiplus::Bitmap& bitmap );

}