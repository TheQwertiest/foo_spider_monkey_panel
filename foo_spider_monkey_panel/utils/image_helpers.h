#pragma once

#include <optional>
#include <string>

namespace smp::image
{

/// @return 0 - error, task id - otherwise
uint32_t LoadImageAsync( HWND hWnd, const std::wstring& imagePath );

/// @return nullptr - error, pointer to loaded image - otherwise
std::unique_ptr<Gdiplus::Bitmap> LoadImage( const std::wstring& imagePath );

} // namespace smp::image
