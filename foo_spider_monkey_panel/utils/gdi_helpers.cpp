#include <stdafx.h>

#include "gdi_helpers.h"

namespace smp::gdi
{

unique_hbitmap CreateHBitmapFromGdiPlusBitmap( Gdiplus::Bitmap& bitmap )
{
    const Gdiplus::Rect rect = { 0, 0, static_cast<INT>( bitmap.GetWidth() ), static_cast<INT>( bitmap.GetHeight() ) };
    Gdiplus::BitmapData bmpdata = {};

    if ( bitmap.LockBits( &rect, Gdiplus::ImageLockModeRead, PixelFormat32bppPARGB, &bmpdata ) != Gdiplus::Ok )
    {
        // Error -> return nullptr
        return unique_hbitmap( nullptr );
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

    return unique_hbitmap( hBitmap );
}

void MakeLogfontW( LOGFONTW& logfontOut,
                   const std::wstring& fontName, const int32_t fontSize, const uint32_t fontStyle )
{
    memset( &logfontOut, 0, sizeof( LOGFONTW ) );

    BOOL font_smoothing = 0;
    UINT smoothing_type = 0;

    SystemParametersInfoW( SPI_GETFONTSMOOTHING, 0, &font_smoothing, FALSE );
    SystemParametersInfoW( SPI_GETFONTSMOOTHINGTYPE, 0, &smoothing_type, FALSE );

    // when 'size = 0' = the font mappers "reasonable default size", meaning 16px em height
    // when 'size > 0' = line height px (tmHeight)  (aka em height)
    // when 'size < 0' = char height px (tmHeight - tmInternalLeading)
    // note: this is flipped against how windows handles lfHeight, as to not break existing scripts
    logfontOut.lfHeight = ( 0 - fontSize );

    logfontOut.lfWeight = ( fontStyle & Gdiplus::FontStyleBold ) ? FW_BOLD : FW_NORMAL;
    logfontOut.lfItalic = ( fontStyle & Gdiplus::FontStyleItalic ) ? TRUE : FALSE;
    logfontOut.lfUnderline = ( fontStyle & Gdiplus::FontStyleUnderline ) ? TRUE : FALSE;
    logfontOut.lfStrikeOut = ( fontStyle & Gdiplus::FontStyleStrikeout ) ? TRUE : FALSE;

    logfontOut.lfCharSet = DEFAULT_CHARSET;
    // logfontOut.lfOutPrecision = OUT_TT_PRECIS;

    logfontOut.lfQuality = //static_cast<BYTE>
    (
        font_smoothing
            ? ( smoothing_type == FE_FONTSMOOTHINGCLEARTYPE
                ? CLEARTYPE_NATURAL_QUALITY
                : ANTIALIASED_QUALITY )
            : DEFAULT_QUALITY
    );

    if ( fontName.length() > 0 )
    {
        fontName.copy( logfontOut.lfFaceName, LF_FACESIZE - 1 );
    }
}

} // namespace smp::gdi
