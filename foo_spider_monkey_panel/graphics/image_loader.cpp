#include <stdafx.h>

#include "image_loader.h"

#include <wincodec.h>

#include <qwr/final_action.h>
#include <qwr/winapi_error_helpers.h>

namespace smp::graphics
{

std::unique_ptr<const LoadedImage> LoadImageFromFile( const std::filesystem::path& path )
{
    qwr::QwrException::ExpectTrue( std::filesystem::exists( path ), "Path does not point to a file" );

    // Note: this prevents the file from being modified or being deleted, but it allows 'delayed' loading
    // and also doesn't block read operations
    qwr::ComPtr<IStream> pStream;
    HRESULT hr = SHCreateStreamOnFileEx( path.c_str(), STGM_READ | STGM_SHARE_DENY_WRITE, GENERIC_READ, false, nullptr, &pStream );
    qwr::error::CheckWin32( hr, "SHCreateStreamOnFileEx" );

    qwr::ComPtr<IWICImagingFactory> pFactory;
    hr = CoCreateInstance( CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_IUnknown, reinterpret_cast<void**>( &pFactory ) );
    qwr::error::CheckWin32( hr, "CoCreateInstance" );

    qwr::ComPtr<IWICBitmapDecoder> pDecoder;
    hr = pFactory->CreateDecoderFromStream( pStream, nullptr, WICDecodeMetadataCacheOnDemand, &pDecoder );
    qwr::error::CheckWin32( hr, "CreateDecoderFromStream" );

    qwr::ComPtr<IWICBitmapFrameDecode> pSource;
    hr = pDecoder->GetFrame( 0, &pSource );
    qwr::error::CheckWin32( hr, "GetFrame" );

    uint32_t width = 0;
    uint32_t height = 0;
    hr = pSource->GetSize( &width, &height );
    qwr::error::CheckWin32( hr, "GetSize" );

    hr = IStream_Reset( pStream );
    qwr::error::CheckWin32( hr, "IStream_Reset" );

    STATSTG stats{};
    hr = pStream->Stat( &stats, STATFLAG_NONAME );
    qwr::error::CheckWin32( hr, "Stat" );

    ULONG bytesRead = 0;
    std::vector<uint8_t> buffer( static_cast<size_t>( stats.cbSize.QuadPart ), 0 );
    hr = pStream->Read( buffer.data(), buffer.size(), &bytesRead );
    qwr::error::CheckWin32( hr, "Read" );
    assert( bytesRead == buffer.size() );

    return std::make_unique<LoadedImage>( width, height, std::move( buffer ) );
}

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

} // namespace smp::graphics
