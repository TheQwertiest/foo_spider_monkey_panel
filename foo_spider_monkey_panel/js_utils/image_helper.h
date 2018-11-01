#pragma once

#include <optional>
#include <string>

namespace mozjs::image
{

/// @details Doesn't report
/// @return 0 - error, task id - otherwise
uint32_t LoadImageAsync( HWND hWnd, const std::wstring& imagePath );

/// @details Doesn't report
/// @return nullptr - error, pointer to loaded image - otherwise
std::unique_ptr<Gdiplus::Bitmap> LoadImage( const std::wstring& imagePath );

}
