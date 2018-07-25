#include <stdafx.h>
#include "image_helper.h"

#include <helpers.h>
#include <user_message.h>

#include <Shlwapi.h>

#include <algorithm>

namespace
{

using namespace mozjs;

class LoadImageTask : public simple_thread_task
{
public:
    LoadImageTask( HWND hNotifyWnd, const std::wstring& imagePath );

private:
    virtual void run();

private:
    HWND hNotifyWnd_;
    std::wstring imagePath_;
};

LoadImageTask::LoadImageTask( HWND hNotifyWnd, const std::wstring& imagePath )
    : hNotifyWnd_( hNotifyWnd )
    , imagePath_( imagePath )
{
}

void LoadImageTask::run()
{
    std::unique_ptr<Gdiplus::Bitmap> bitmap( new Gdiplus::Bitmap( imagePath_.c_str(), PixelFormat32bppPARGB ) );
    if ( !helpers::ensure_gdiplus_object( bitmap.get() ) )
    {
        bitmap.reset();
    }

    image::AsyncImageTaskResult taskResult;
    taskResult.bitmap.swap( bitmap );

    taskResult.imagePath = imagePath_.empty() ? "" : file_path_display( pfc::stringcvt::string_utf8_from_wide( imagePath_.c_str(), imagePath_.length() ) );
    SendMessage( hNotifyWnd_, CALLBACK_UWM_ON_LOAD_IMAGE_DONE, 0, (LPARAM)&taskResult );
}

}

namespace mozjs::image
{

uint32_t LoadImageAsync( HWND hWnd, const std::wstring& imagePath )
{
    try
    {
        std::unique_ptr<LoadImageTask> task( new LoadImageTask( hWnd, imagePath ) );
        uint32_t taskId = [&]()
        {
            uint64_t tmp = reinterpret_cast<uint64_t>(task.get());
            return static_cast<uint32_t>((tmp & 0xFFFFFFFF) ^ (tmp >> 32));
        }();

        if ( simple_thread_pool::instance().enqueue( std::move( task ) ) )
        {
            return taskId;
        }
    }
    catch ( ... )
    {
    }

    return 0;
}

}
