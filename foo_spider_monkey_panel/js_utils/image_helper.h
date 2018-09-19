#pragma once

#include <optional>
#include <string>

namespace mozjs::image
{

struct AsyncImageTaskResult
{
    uint32_t taskId;
    std::unique_ptr<Gdiplus::Bitmap> bitmap;
    pfc::string8_fast imagePath;
};

/// @details Doesn't report
/// @return 0 - error, task id - otherwise
uint32_t LoadImageAsync( HWND hWnd, const std::wstring& imagePath );

/// @details Doesn't report
/// @return nullptr - error, pointer to loaded image - otherwise
std::unique_ptr<Gdiplus::Bitmap> LoadImage( const std::wstring& imagePath );

}
