#include <stdafx.h>

#include "gdi_helpers.h"

namespace smp::gdi
{

unique_gdi_ptr<HBITMAP> CreateHBitmapFromGdiPlusBitmap( Gdiplus::Bitmap& bitmap )
{
    const Gdiplus::Rect rect{ 0, 0, static_cast<int>( bitmap.GetWidth() ), static_cast<int>( bitmap.GetHeight() ) };
    Gdiplus::BitmapData bmpdata{};

    if ( bitmap.LockBits( &rect, Gdiplus::ImageLockModeRead, PixelFormat32bppPARGB, &bmpdata ) != Gdiplus::Ok )
    { // Error
        return CreateUniquePtr( HBITMAP( nullptr ) );
    }

    BITMAP bm{};
    bm.bmType = 0;
    bm.bmWidth = bmpdata.Width;
    bm.bmHeight = bmpdata.Height;
    bm.bmWidthBytes = bmpdata.Stride;
    bm.bmPlanes = 1;
    bm.bmBitsPixel = 32;
    bm.bmBits = bmpdata.Scan0;

    HBITMAP hBitmap = CreateBitmapIndirect( &bm );
    bitmap.UnlockBits( &bmpdata );

    return CreateUniquePtr( hBitmap );
}

} // namespace smp::gdi
