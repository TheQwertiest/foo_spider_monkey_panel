#include <stdafx.h>

#include "gdi_helpers.h"

namespace smp::gdi
{

unique_gdi_ptr<HBITMAP> CreateHBitmapFromGdiPlusBitmap( Gdiplus::Bitmap& bitmap )
{
    const Gdiplus::Rect rect{ 0, 0, static_cast<INT>( bitmap.GetWidth() ), static_cast<INT>( bitmap.GetHeight() ) };
    Gdiplus::BitmapData bmpdata{};

    if ( bitmap.LockBits( &rect, Gdiplus::ImageLockModeRead, PixelFormat32bppPARGB, &bmpdata ) != Gdiplus::Ok )
        return unique_gdi_ptr<HBITMAP>( nullptr ); // Error -> return null handle

    BITMAP bm
    {
        0,                                      // bmType
        static_cast<LONG>( bmpdata.Width ),     // bmWidth
        static_cast<LONG>( bmpdata.Height ),    // bmheight
        bmpdata.Stride,                         // bmWidthBytes
        1,                                      // bmPlanes
        32,                                     // bmBitsPixel
        bmpdata.Scan0                           // bmBits
    };

    HBITMAP hBitmap = CreateBitmapIndirect( &bm );
    bitmap.UnlockBits( &bmpdata );

    return unique_gdi_ptr<HBITMAP>( hBitmap );
}

} // namespace smp::gdi
