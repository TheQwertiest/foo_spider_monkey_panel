#pragma once

#include <optional>
#include <string>

namespace mozjs::image
{

struct AsyncImageTaskResult
{
    std::unique_ptr<Gdiplus::Bitmap> bitmap;
    pfc::string8_fast imagePath;
};

/// @details Doesn't report errors
/// @return 0 - error, task id - otherwise
uint32_t LoadImageAsync( HWND hWnd, const std::wstring& imagePath );

/// @details Doesn't report errors
/// @return nullptr - error, pointer to loaded image - otherwise
std::unique_ptr<Gdiplus::Bitmap> LoadImage( const std::wstring& imagePath );

}
