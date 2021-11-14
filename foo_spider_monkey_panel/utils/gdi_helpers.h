#pragma once

#include <windef.h>

#include <memory>

namespace smp::gdi
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

// TODO: wil/resource.h ?

template <typename T>
using unique_gdi_ptr = std::unique_ptr<std::remove_pointer_t<T>, void ( * )( T )>;

template <typename T>
[[nodiscard]] unique_gdi_ptr<T> CreateUniquePtr( T pObject )
{
    static_assert
    (
        std::is_same_v<T, HDC> ||
        std::is_same_v<T, HPEN> ||
        std::is_same_v<T, HBRUSH> ||
        std::is_same_v<T, HRGN> ||
        std::is_same_v<T, HPALETTE> ||
        std::is_same_v<T, HFONT> ||
        std::is_same_v<T, HBITMAP>,
        "Unsupported GDI type"
    );

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

template <typename T>
class ObjectSelector
{
    static_assert
    (
        std::is_same_v<T, HPEN> ||
        std::is_same_v<T, HBRUSH> ||
        std::is_same_v<T, HFONT> ||
        std::is_same_v<T, HBITMAP>,
        "Unsupported GDI type"
    );

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

/// @details Does not report
/// @return nullptr - error, create HBITMAP - otherwise
[[nodiscard]] unique_gdi_ptr<HBITMAP> CreateHBitmapFromGdiPlusBitmap( Gdiplus::Bitmap& bitmap );

} // namespace smp::gdi
