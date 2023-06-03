#include <stdafx.h>

#include "bitmap_generator.h"

#include <utils/gdi_error_helpers.h>
#include <utils/guid_helpers.h>

#include <wincodec.h>

#include <qwr/com_ptr.h>
#include <qwr/final_action.h>
#include <qwr/winapi_error_helpers.h>

namespace
{

const std::unordered_map<GUID, int, smp::utils::GuidHasher> kWicFormatToGdiPlusFormat = {
    { GUID_WICPixelFormat1bppIndexed, PixelFormat1bppIndexed },
    { GUID_WICPixelFormat4bppIndexed, PixelFormat4bppIndexed },
    { GUID_WICPixelFormat8bppIndexed, PixelFormat8bppIndexed },
    { GUID_WICPixelFormat8bppIndexed, PixelFormat16bppGrayScale },
    { GUID_WICPixelFormat16bppBGR555, PixelFormat16bppRGB555 },
    { GUID_WICPixelFormat16bppBGR565, PixelFormat16bppRGB565 },
    { GUID_WICPixelFormat16bppBGRA5551, PixelFormat16bppARGB1555 },
    { GUID_WICPixelFormat24bppBGR, PixelFormat24bppRGB },
    { GUID_WICPixelFormat32bppBGR, PixelFormat32bppRGB },
    { GUID_WICPixelFormat32bppBGRA, PixelFormat32bppARGB },
    { GUID_WICPixelFormat32bppPBGRA, PixelFormat32bppPARGB },
    { GUID_WICPixelFormat48bppBGR, PixelFormat48bppRGB },
    { GUID_WICPixelFormat64bppBGRA, PixelFormat64bppARGB },
    { GUID_WICPixelFormat64bppPBGRA, PixelFormat64bppPARGB },
    { GUID_WICPixelFormat32bppCMYK, PixelFormat32bppCMYK },
};

}

namespace smp::graphics
{

std::unique_ptr<Gdiplus::Bitmap> GenerateGdiBitmap( IWICBitmap& wicBitmap )
{
    WICPixelFormatGUID pixelFormat;
    auto hr = wicBitmap.GetPixelFormat( &pixelFormat );
    qwr::error::CheckHR( hr, "GetPixelFormat" );

    qwr::ComPtr<IWICBitmapSource> pConvertedSource;
    if ( !kWicFormatToGdiPlusFormat.contains( pixelFormat ) )
    { // convert only when necessary
        pixelFormat = GUID_WICPixelFormat32bppPBGRA;

        hr = WICConvertBitmapSource( pixelFormat, &wicBitmap, &pConvertedSource );
        qwr::error::CheckHR( hr, "WICConvertBitmapSource" );
    }
    auto& wicBitmapRef = ( pConvertedSource ? *pConvertedSource : wicBitmap );
    const auto gdiFormat = kWicFormatToGdiPlusFormat.at( pixelFormat );

    uint32_t width = 0;
    uint32_t height = 0;
    hr = wicBitmap.GetSize( &width, &height );
    qwr::error::CheckHR( hr, "GetSize" );

    auto pGdiBitmap = std::make_unique<Gdiplus::Bitmap>( width, height, gdiFormat );
    smp::error::CheckGdiPlusObject( pGdiBitmap );

    Gdiplus::Rect gdiRect{ 0, 0, static_cast<int32_t>( width ), static_cast<int32_t>( height ) };
    Gdiplus::BitmapData bmpData{};
    auto gdiRet = pGdiBitmap->LockBits( &gdiRect, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite, gdiFormat, &bmpData );
    smp::error::CheckGdi( gdiRet, "LockBits" );
    {
        qwr::final_action autoLockBits( [&] { pGdiBitmap->UnlockBits( &bmpData ); } );

        hr = wicBitmap.CopyPixels( nullptr, bmpData.Stride, bmpData.Stride * bmpData.Height, static_cast<uint8_t*>( bmpData.Scan0 ) );
        qwr::error::CheckHR( hr, "CopyPixels" );
    }

    return pGdiBitmap;
}

} // namespace smp::graphics
