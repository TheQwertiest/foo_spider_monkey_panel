#include <stdafx.h>

#include "image_loader.h"

#include <wincodec.h>

#include <qwr/final_action.h>
#include <qwr/winapi_error_helpers.h>

namespace smp::graphics
{

LoadedImage LoadImageFromFile( const std::filesystem::path& path )
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

    qwr::ComPtr<IStream> pStream2;
    hr = pStream->Commit( 0 );
    qwr::error::CheckWin32( hr, "Clone" );

    return { width, height, pStream };
}

qwr::ComPtr<IWICBitmap> DecodeImage( const LoadedImage& loadedImage )
{
    // Must be only used on main thread, because we can't use pStream in parallel (can't clone it either).
    // Revisit if parallel use is needed
    assert( core_api::is_main_thread() );

    qwr::ComPtr<IWICImagingFactory> pFactory;
    auto hr = CoCreateInstance( CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_IUnknown, reinterpret_cast<void**>( &pFactory ) );
    qwr::error::CheckWin32( hr, "CoCreateInstance" );

    qwr::ComPtr<IWICBitmapDecoder> pDecoder;
    hr = pFactory->CreateDecoderFromStream( loadedImage.pDataStream, nullptr, WICDecodeMetadataCacheOnDemand, &pDecoder );
    qwr::error::CheckWin32( hr, "CreateDecoderFromStream" );

    qwr::ComPtr<IWICBitmapFrameDecode> pSource;
    hr = pDecoder->GetFrame( 0, &pSource );
    qwr::error::CheckWin32( hr, "GetFrame" );

    qwr::ComPtr<IWICBitmap> pBitmap;
    hr = pFactory->CreateBitmapFromSource( pSource, WICBitmapCacheOnLoad, &pBitmap );
    qwr::error::CheckWin32( hr, "CreateBitmapFromSource" );

    hr = IStream_Reset( loadedImage.pDataStream );
    qwr::error::CheckWin32( hr, "IStream_Reset" );

    return pBitmap;
}

} // namespace smp::graphics
