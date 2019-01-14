#include <stdafx.h>
#include "image_helpers.h"

#include <utils/gdi_helpers.h>
#include <utils/thread_pool.h>

#include <helpers.h>
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

    uint32_t GetTaskId() const;

private:
    void run();

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

    std::unique_ptr<Gdiplus::Bitmap> img( new Gdiplus::Bitmap( pStream, PixelFormat32bppPARGB ) );
    if ( !gdi::IsGdiPlusObjectValid( img ) )
    {
        return nullptr;
    }

    return img;
}

} // namespace smp::image
