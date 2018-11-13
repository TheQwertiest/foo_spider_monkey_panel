#pragma once

#include <windef.h>

#include <memory>

namespace mozjs::gdi
{

template <typename T>
bool IsGdiPlusObjectValid( const T* obj )
{
    return ( obj && ( Gdiplus::Ok == obj->GetLastStatus() ) );
}

template <typename T>
bool IsGdiPlusObjectValid( const std::unique_ptr<T>& obj )
{
    return IsGdiPlusObjectValid( obj.get() );
}

template <typename T>
using unique_gdi_ptr = std::unique_ptr<std::remove_pointer_t<T>, void ( * )( T )>;

template <typename T>
unique_gdi_ptr<T> CreateUniquePtr( T pObject )
{
    static_assert( std::is_same_v<T, HBITMAP> 
                   || std::is_same_v<T, HDC> 
                   || std::is_same_v<T, HFONT> 
                   || std::is_same_v<T, HBRUSH>,
                   "Unsupported type" );

    return unique_gdi_ptr<T>( pObject, []( auto pObject ) { DeleteObject( pObject ); } );
}

/// @details Does not report
/// @return nullptr - error, create HBITMAP - otherwise
unique_gdi_ptr<HBITMAP> CreateHBitmapFromGdiPlusBitmap( Gdiplus::Bitmap& bitmap );

} // namespace mozjs::gdi
