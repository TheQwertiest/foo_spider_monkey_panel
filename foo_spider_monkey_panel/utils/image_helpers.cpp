#include <stdafx.h>

#include "image_helpers.h"

#include <events/event_dispatcher.h>
#include <events/event_js_callback.h>
#include <utils/gdi_helpers.h>
#include <utils/guid_helpers.h>
#include <utils/thread_pool_instance.h>

#include <Shlwapi.h>
#include <wincodec.h>

#include <qwr/final_action.h>

#include <algorithm>

namespace
{

using namespace smp;

class LoadImageTask
{
public:
    LoadImageTask( HWND hNotifyWnd, uint32_t taskId, const std::wstring& imagePath );

    LoadImageTask( const LoadImageTask& ) = delete;
    LoadImageTask& operator=( const LoadImageTask& ) = delete;

    void operator()();

    [[nodiscard]] uint32_t GetTaskId() const;

private:
    void run();

private:
    HWND hNotifyWnd_;
    uint32_t taskId_{};
    std::wstring imagePath_;
};

LoadImageTask::LoadImageTask( HWND hNotifyWnd, uint32_t taskId, const std::wstring& imagePath )
    : hNotifyWnd_( hNotifyWnd )
    , taskId_( taskId )
    , imagePath_( imagePath )
{
}

void LoadImageTask::operator()()
{
    return run();
}

uint32_t LoadImageTask::GetTaskId() const
{
    return taskId_;
}

void LoadImageTask::run()
{
    const qwr::u8string path = file_path_display( qwr::unicode::ToU8( imagePath_ ).c_str() ).get_ptr();

    EventDispatcher::Get().PutEvent( hNotifyWnd_,
                                     GenerateEvent_JsCallback(
                                         EventId::kInternalLoadImageDone,
                                         taskId_,
                                         image::LoadImage( imagePath_ ),
                                         path ) );
}

} // namespace

namespace smp::image
{

std::tuple<uint32_t, uint32_t>
GetResizedImageSize( const std::tuple<uint32_t, uint32_t>& currentDimension,
                     const std::tuple<uint32_t, uint32_t>& maxDimensions ) noexcept
{
    const auto& [maxWidth, maxHeight] = maxDimensions;
    const auto& [imgWidth, imgHeight] = currentDimension;

    uint32_t newWidth;
    uint32_t newHeight;
    if ( imgWidth <= maxWidth && imgHeight <= maxHeight )
    {
        newWidth = imgWidth;
        newHeight = imgHeight;
    }
    else
    {
        const double imgRatio = static_cast<double>( imgHeight ) / imgWidth;
        const double constraintsRatio = static_cast<double>( maxHeight ) / maxWidth;
        if ( imgRatio > constraintsRatio )
        {
            newHeight = maxHeight;
            newWidth = lround( newHeight / imgRatio );
        }
        else
        {
            newWidth = maxWidth;
            newHeight = lround( newWidth * imgRatio );
        }
    }

    return std::make_tuple( newWidth, newHeight );
}

uint32_t LoadImageAsync( HWND hWnd, const std::wstring& imagePath )
{
    // This is performed on the main thread only, so it's all good
    static uint32_t g_taskId = 0;
    if ( g_taskId == std::numeric_limits<uint32_t>::max() )
    {
        g_taskId = 0;
    }

    auto task = std::make_shared<LoadImageTask>( hWnd, g_taskId++, imagePath );
    smp::GetThreadPoolInstance().AddTask( [task] { std::invoke( *task ); } );

    return task->GetTaskId();
}

std::unique_ptr<Gdiplus::Bitmap> LoadImage( const std::wstring& imagePath )
{
    // Note: this prevents the file from being modified or being deleted, but it allows 'delayed' loading
    IStreamPtr pStream;
    auto hr = SHCreateStreamOnFileEx( imagePath.c_str(), STGM_READ | STGM_SHARE_DENY_WRITE, GENERIC_READ, FALSE, nullptr, &pStream );
    if ( FAILED( hr ) )
    {
        return nullptr;
    }

    auto pImg = std::make_unique<Gdiplus::Bitmap>( static_cast<IStream*>( pStream ), TRUE );
    if ( !gdi::IsGdiPlusObjectValid( pImg ) )
    {
        return LoadImageWithWIC( pStream );
    }

    return pImg;
}

std::unique_ptr<Gdiplus::Bitmap> LoadImageWithWIC( IStreamPtr pStream )
{
    // TODO: replace nullptr with proper exception handling
    pfc::com_ptr_t<IWICImagingFactory> pFactory;
    auto hr = CoCreateInstance( CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_IUnknown, pFactory.receive_void_ptr() );
    if ( FAILED( hr ) )
    {
        return nullptr;
    }

    pfc::com_ptr_t<IWICBitmapDecoder> pDecoder;
    hr = pFactory->CreateDecoderFromStream(
        pStream, nullptr, WICDecodeMetadataCacheOnDemand, pDecoder.receive_ptr() );
    if ( FAILED( hr ) )
    {
        return nullptr;
    }

    pfc::com_ptr_t<IWICBitmapFrameDecode> pFrame;
    hr = pDecoder->GetFrame( 0, pFrame.receive_ptr() );
    if ( FAILED( hr ) )
    {
        return nullptr;
    }

    pfc::com_ptr_t<IWICBitmapSource> pSource = pFrame;

    WICPixelFormatGUID pixelFormat;
    hr = pSource->GetPixelFormat( &pixelFormat );
    if ( FAILED( hr ) )
    {
        return nullptr;
    }

    static const std::unordered_map<GUID, int, smp::utils::GuidHasher> wicFormatToGdiPlusFormat = {
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
    if ( !wicFormatToGdiPlusFormat.contains( pixelFormat ) )
    { // convert only when necessary
        pixelFormat = GUID_WICPixelFormat32bppPBGRA;

        pfc::com_ptr_t<IWICBitmapSource> pConvertedSource;
        hr = WICConvertBitmapSource( pixelFormat, pSource.get_ptr(), pConvertedSource.receive_ptr() );
        if ( FAILED( hr ) )
        {
            return nullptr;
        }

        pSource = pConvertedSource;
    }

    assert( wicFormatToGdiPlusFormat.contains( pixelFormat ) );
    const auto gdiFormat = wicFormatToGdiPlusFormat.at( pixelFormat );

    UINT h;
    UINT w;
    hr = pSource->GetSize( &w, &h );
    if ( FAILED( hr ) )
    {
        return nullptr;
    }

    auto pGdiBitmap = std::make_unique<Gdiplus::Bitmap>( w, h, gdiFormat );
    if ( !pGdiBitmap || Gdiplus::Status::Ok != pGdiBitmap->GetLastStatus() )
    {
        return nullptr;
    }

    Gdiplus::Rect rect{ 0, 0, static_cast<INT>( w ), static_cast<INT>( h ) };
    Gdiplus::BitmapData bmpdata{};
    auto gdiStatus = pGdiBitmap->LockBits( &rect, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite, gdiFormat, &bmpdata );
    if ( gdiStatus != Gdiplus::Ok )
    {
        return nullptr;
    }

    {
        // unlock bits before returning
        qwr::final_action autoDstBits( [&pGdiBitmap, &bmpdata] { pGdiBitmap->UnlockBits( &bmpdata ); } );

        hr = pSource->CopyPixels( nullptr, bmpdata.Stride, bmpdata.Stride * bmpdata.Height, static_cast<uint8_t*>( bmpdata.Scan0 ) );
        if ( FAILED( hr ) )
        {
            return nullptr;
        }
    }

    return pGdiBitmap;
}

} // namespace smp::image
