#include <stdafx.h>

#include "image_manager.h"

#include <wincodec.h>

#include <qwr/winapi_error_helpers.h>

namespace fs = std::filesystem;

namespace
{

/// @throw qwr::QwrException
smp::not_null_shared<const smp::LoadedImage> FetchImageFromStream( qwr::ComPtr<IStream> pStream )
{
    qwr::ComPtr<IWICImagingFactory> pFactory;
    auto hr = CoCreateInstance( CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_IUnknown, reinterpret_cast<void**>( &pFactory ) );
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

    return smp::make_not_null_shared<const smp::LoadedImage>( width, height, std::move( buffer ) );
}

/// @throw qwr::QwrException
smp::not_null_shared<const smp::LoadedImage> FetchImageFromFile( const std::filesystem::path& path )
{
    qwr::QwrException::ExpectTrue( std::filesystem::exists( path ), "Path does not point to a file" );

    // Note: this prevents the file from being modified or being deleted, but it allows 'delayed' loading
    // and also doesn't block read operations
    qwr::ComPtr<IStream> pStream;
    HRESULT hr = SHCreateStreamOnFileEx( path.c_str(), STGM_READ | STGM_SHARE_DENY_WRITE, GENERIC_READ, false, nullptr, &pStream );
    qwr::error::CheckWin32( hr, "SHCreateStreamOnFileEx" );

    return FetchImageFromStream( pStream );
}

/// @throw qwr::QwrException
smp::not_null_shared<const smp::LoadedImage> FetchImageFromData( std::span<const uint8_t> imageData )
{
    auto pMemStream = SHCreateMemStream( imageData.data(), imageData.size() );
    qwr::error::CheckWinApi( pMemStream, "SHCreateMemStream" );

    // copy and assignment operators increase Stream ref count,
    // while SHCreateMemStream returns object with ref count 1,
    // so we need to take ownership without increasing ref count
    // (or decrease ref count manually)
    qwr::ComPtr<IStream> pStream;
    pStream.Attach( pMemStream );

    return FetchImageFromStream( pStream );
}

} // namespace

namespace smp
{

ImageManager::ImageManager()
    // TODO: replace with config
    : uriToImage_( 10 )
{
}

ImageManager& ImageManager::Get()
{
    static ImageManager cache;
    return cache;
}

std::shared_ptr<const LoadedImage> ImageManager::GetCached( const std::wstring& uri ) const
{
    std::scoped_lock lock( mutex_ );

    if ( uriToImage_.Contains( uri ) )
    {
        return uriToImage_.Get( uri );
    }

    if ( uriToImageWeak_.contains( uri ) )
    { // reuse if value is still available
        auto pWeakImage = uriToImageWeak_.at( uri );
        if ( auto pImage = pWeakImage.lock() )
        {
            auto pSharedImage = not_null_shared<const LoadedImage>( pImage );
            return const_cast<ImageManager*>( this )->MaybeCache( uri, pSharedImage );
        }
    }

    return nullptr;
}

not_null_shared<const LoadedImage>
ImageManager::Load( const std::filesystem::path& path )
{
    return FetchImageFromFile( path );
}

not_null_shared<const LoadedImage>
ImageManager::Load( std::span<const uint8_t> data )
{
    return FetchImageFromData( data );
}

not_null_shared<const LoadedImage>
ImageManager::MaybeCache( const std::wstring& uri, not_null_shared<const LoadedImage> pImage )
{
    std::scoped_lock lock( mutex_ );

    if ( uriToImage_.Contains( uri ) )
    { // means that it was loaded by someone else,
        // hence should discard incoming image and return the saved one
        return uriToImage_.Get( uri );
    }

    uriToImageWeak_.try_emplace( uri, pImage.get() );

    // TODO: move to config
    // don't cache huge images
    if ( pImage->rawData.size() <= 50 * 1000 * 1000LL )
    {
        uriToImage_.Put( uri, pImage );
    }

    return pImage;
}

void ImageManager::ClearCache()
{
    uriToImage_.Clear();
    uriToImageWeak_.clear();
}

} // namespace smp
