#include <stdafx.h>
#include "image_helpers.h"

#include <utils/gdi_helpers.h>
#include <utils/thread_pool.h>

#include <user_message.h>
#include <message_manager.h>

#include <Shlwapi.h>

#include <algorithm>

namespace
{

using namespace smp;

class LoadImageTask
{
public:
    LoadImageTask( HWND hNotifyWnd, const std::wstring& imagePath );

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

LoadImageTask::LoadImageTask( HWND hNotifyWnd, const std::wstring& imagePath )
    : hNotifyWnd_( hNotifyWnd )
    , imagePath_( imagePath )
{
    // Such cast will provide unique id only on x86
    taskId_ = reinterpret_cast<uint32_t>( this );
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
    const std::u8string path = file_path_display( smp::unicode::ToU8( imagePath_).c_str() ).get_ptr();
    panel::message_manager::instance().post_callback_msg( hNotifyWnd_,
                                                          smp::CallbackMessage::internal_load_image_done,
                                                          std::make_unique<
                                                              smp::panel::CallbackDataImpl<
                                                                  uint32_t,
                                                                  std::unique_ptr<Gdiplus::Bitmap>,
                                                                  std::u8string>>( taskId_,
                                                                                       image::LoadImage( imagePath_ ),
                                                                                       path ) );
}

} // namespace

namespace smp::image
{

uint32_t LoadImageAsync( HWND hWnd, const std::wstring& imagePath )
{
    // We need to wrap task to keep taskId unique, since it's derived from object address
    auto task = std::make_shared<LoadImageTask>( hWnd, imagePath );
    const uint32_t taskId = task->GetTaskId();

    ThreadPool::GetInstance().AddTask( [task] {
        std::invoke( *task );
    } );

    return taskId;
}

std::unique_ptr<Gdiplus::Bitmap> LoadImage( const std::wstring& imagePath )
{
    // Gdiplus::Bitmap(path) locks file, thus using IStream instead to prevent it.
    IStreamPtr pStream;
    HRESULT hr = SHCreateStreamOnFileEx( imagePath.c_str(), STGM_READ | STGM_SHARE_DENY_WRITE, GENERIC_READ, FALSE, nullptr, &pStream );
    if ( FAILED( hr ) )
    {
        return nullptr;
    }

    std::unique_ptr<Gdiplus::Bitmap> img( new Gdiplus::Bitmap( static_cast<IStream*>( pStream ), TRUE ) );
    if ( !gdi::IsGdiPlusObjectValid( img ) )
    {
        return nullptr;
    }

    return img;
}

std::tuple<uint32_t, uint32_t>
GetResizedImageSize( const std::tuple<uint32_t, uint32_t>& currentDimension,
                     const std::tuple<uint32_t, uint32_t>& maxDimensions ) noexcept
{
    const auto [maxWidth, maxHeight] = maxDimensions;
    const auto [imgWidth, imgHeight] = currentDimension;

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

} // namespace smp::image
