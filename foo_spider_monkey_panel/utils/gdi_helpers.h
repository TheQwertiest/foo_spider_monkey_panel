#pragma once

#include <windef.h>

#include <qwr/type_traits.h>

#include <memory>

namespace smp::gdi
{

namespace impl
{

template <class T>
concept SupportedGdiType = qwr::is_any_same_v<T, HDC, HPEN, HBRUSH, HRGN, HPALETTE, HFONT, HBITMAP>;

} // namespace impl

template <typename T>
using unique_gdi_ptr = std::unique_ptr<std::remove_pointer_t<T>, void ( * )( T )>;

template <impl::SupportedGdiType T>
[[nodiscard]] unique_gdi_ptr<T> CreateUniquePtr( T pObject )
{
    return unique_gdi_ptr<T>( pObject, []( auto pObject ) {
        if constexpr ( std::is_same_v<T, HDC> )
        {
            DeleteDC( pObject );
        }
        else
        {
            DeleteObject( pObject );
        }
    } );
}

template <impl::SupportedGdiType T>
class ObjectSelector
{
public:
    [[nodiscard]] ObjectSelector( HDC hDc, T pNewObject )
        : hDc_( hDc )
        , pOldObject_( SelectObject( hDc, pNewObject ) )
    {
    }

    ~ObjectSelector()
    {
        (void)SelectObject( hDc_, pOldObject_ );
    }

private:
    HDC hDc_ = nullptr;
    HGDIOBJ pOldObject_ = nullptr;
};

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

/// @details Does not report
/// @return nullptr - error, create HBITMAP - otherwise
[[nodiscard]] unique_gdi_ptr<HBITMAP> CreateHBitmapFromGdiPlusBitmap( Gdiplus::Bitmap& bitmap );

} // namespace smp::gdi
