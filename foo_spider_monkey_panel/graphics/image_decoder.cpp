#include <stdafx.h>

#include "image_decoder.h"

#include <graphics/loaded_image.h>

#include <wincodec.h>

#include <qwr/final_action.h>
#include <qwr/winapi_error_helpers.h>

namespace smp
{

qwr::ComPtr<IWICBitmap> DecodeImage( const LoadedImage& loadedImage )
{
    auto pStream = [&] {
        auto pMemStreamRaw = SHCreateMemStream( loadedImage.rawData.data(), loadedImage.rawData.size() );
        qwr::error::CheckWinApi( !!pMemStreamRaw, "SHCreateMemStream" );

        // copy and assignment operators increase Stream ref count,
        // while SHCreateMemStream returns object with ref count 1,
        // so we need to take ownership without increasing ref count
        // (or decrease ref count manually)
        qwr::ComPtr<IStream> pStream;
        pStream.Attach( pMemStreamRaw );
        return pStream;
    }();

    qwr::ComPtr<IWICImagingFactory> pFactory;
    auto hr = CoCreateInstance( CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_IUnknown, reinterpret_cast<void**>( &pFactory ) );
    qwr::error::CheckWin32( hr, "CoCreateInstance" );

    qwr::ComPtr<IWICBitmapDecoder> pDecoder;
    hr = pFactory->CreateDecoderFromStream( pStream, nullptr, WICDecodeMetadataCacheOnDemand, &pDecoder );
    qwr::error::CheckWin32( hr, "CreateDecoderFromStream" );

    qwr::ComPtr<IWICBitmapFrameDecode> pSource;
    hr = pDecoder->GetFrame( 0, &pSource );
    qwr::error::CheckWin32( hr, "GetFrame" );

    qwr::ComPtr<IWICBitmap> pBitmap;
    hr = pFactory->CreateBitmapFromSource( pSource, WICBitmapCacheOnLoad, &pBitmap );
    qwr::error::CheckWin32( hr, "CreateBitmapFromSource" );

    return pBitmap;
}

} // namespace smp
