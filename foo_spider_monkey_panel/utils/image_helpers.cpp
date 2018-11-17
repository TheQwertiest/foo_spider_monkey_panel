#include <stdafx.h>
#include "image_helpers.h"

#include <utils/gdi_helpers.h>

#include <helpers.h>
#include <user_message.h>
#include <message_manager.h>

#include <Shlwapi.h>

#include <algorithm>

namespace
{

using namespace smp;

class LoadImageTask : public simple_thread_task
{
public:
    LoadImageTask( HWND hNotifyWnd, const std::wstring& imagePath );

    uint32_t GetTaskId() const;

private:
    void run() override;

private:
    HWND hNotifyWnd_;
    uint32_t taskId_;
    std::wstring imagePath_;
};

LoadImageTask::LoadImageTask( HWND hNotifyWnd, const std::wstring& imagePath )
    : hNotifyWnd_( hNotifyWnd )
    , imagePath_( imagePath )
{
    // Such cast will provide unique id only on x86
    taskId_ = reinterpret_cast<uint32_t>( this );
}

uint32_t LoadImageTask::GetTaskId() const
{
    return taskId_;
}

void LoadImageTask::run()
{
    const pfc::string8_fast path = file_path_display( pfc::stringcvt::string_utf8_from_wide( imagePath_.c_str(), imagePath_.length() ) );
    panel::message_manager::instance().post_callback_msg( hNotifyWnd_,
                                                        smp::CallbackMessage::internal_load_image_done,
                                                        std::make_unique<
                                                            smp::panel::CallbackDataImpl<
                                                                uint32_t,
                                                                std::unique_ptr<Gdiplus::Bitmap>,
                                                                pfc::string8_fast>>( taskId_,
                                                                                     std::move( image::LoadImage( imagePath_ ) ),
                                                                                     path ) );
}

} // namespace

namespace smp::image
{

uint32_t LoadImageAsync( HWND hWnd, const std::wstring& imagePath )
{
    try
    {
        std::unique_ptr<LoadImageTask> task( new LoadImageTask( hWnd, imagePath ) );
        const uint32_t taskId = task->GetTaskId();

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

std::unique_ptr<Gdiplus::Bitmap> LoadImage( const std::wstring& imagePath )
{
    // Since using Gdiplus::Bitmap(path) will result locking file, using IStream instead to prevent it.
    IStreamPtr pStream;
    HRESULT hr = SHCreateStreamOnFileEx( imagePath.c_str(), STGM_READ | STGM_SHARE_DENY_WRITE, GENERIC_READ, FALSE, nullptr, &pStream );
    if ( !SUCCEEDED( hr ) )
    {
        return nullptr;
    }

    std::unique_ptr<Gdiplus::Bitmap> img( new Gdiplus::Bitmap( pStream, PixelFormat32bppPARGB ) );
    if ( !gdi::IsGdiPlusObjectValid( img ) )
    {
        return nullptr;
    }

    return img;
}

} // namespace smp::image
