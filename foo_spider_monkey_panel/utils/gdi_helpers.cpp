#include <stdafx.h>

#include "gdi_helpers.h"

#include <utils/gdi_error_helpers.h>

#include <qwr/final_action.h>
#include <qwr/winapi_error_helpers.h>

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

// wrap GDI calls inside GDI+ blocks, while transferring clipping regions and transforms
void WrapGdiCalls( Gdiplus::Graphics* graphics, std::function<void( HDC dc )> const& GdiOnlyDrawer )
{
    qwr::QwrException::ExpectTrue( GdiFlush(), "WrapGdiCalls: pre-GdiFlush failed" );
    qwr::QwrException::ExpectTrue( graphics, "Internal error: Gdiplus::Graphics object is null" );

    // get current clip region and transform
    // we need to do this before getting the dc
    Gdiplus::Region region = {};
    qwr::error::CheckGdi( graphics->GetClip( &region ), "GetClip" );
    HRGN hrgn{ region.GetHRGN( graphics ) };

    Gdiplus::Matrix matrix = {};
    qwr::error::CheckGdi( graphics->GetTransform( &matrix ), "GetTransform" );

    XFORM xform = {};
    qwr::error::CheckGdi( matrix.GetElements( (Gdiplus::REAL*)( &xform ) ), "GetElements" );

    // get the device context
    // no Gdiplus draw-calls beyond this point until releaseDC
    // see: https://docs.microsoft.com/en-GB/troubleshoot/windows/win32/mix-gdi-and-gdi-plus-drawing
    const HDC dc = graphics->GetHDC();
    qwr::final_action autoReleaseDc( [graphics, dc] { graphics->ReleaseHDC( dc ); } );

    // set clip and transform
    HRGN oldhrgn = CreateRectRgn( 0, 0, 0, 0 ); // dummy region for getting the current clip region into
    auto oldclip = GetClipRgn( dc, oldhrgn );

    qwr::error::CheckWinApi( GDI_ERROR != oldclip, "GetClipRgn" );
    qwr::final_action autoReleaseRegion( [dc, oldhrgn, oldclip]
    {
        if ( RGN_ERROR != oldclip )
            SelectClipRgn( dc, oldhrgn );
        ::DeleteObject( oldhrgn );
    } );

    qwr::error::CheckWinApi( ERROR != ExtSelectClipRgn( dc, hrgn, RGN_AND ), "SelectClipRgn" );
    DeleteObject( hrgn );

    XFORM oldxform = {};
    qwr::error::CheckWinApi( ERROR != GetWorldTransform( dc, &oldxform ), "GetWorldTransform" );
    qwr::final_action autoResetTansform( [dc, oldxform]() { SetWorldTransform( dc, &oldxform ); } );

    qwr::error::CheckWinApi( ERROR != SetWorldTransform( dc, &xform ), "SetWorldTransform" );

    // and finally:
    GdiOnlyDrawer( dc );

    qwr::QwrException::ExpectTrue( GdiFlush(), "WrapGdiCalls: post-GdiFlush failed" );
}

} // namespace smp::gdi
