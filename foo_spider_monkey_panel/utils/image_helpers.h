#pragma once

#include <optional>
#include <string>
#include <tuple>

namespace smp::image
{

std::tuple<uint32_t, uint32_t>
GetResizedImageSize( const std::tuple<uint32_t, uint32_t>& currentDimension,
                     const std::tuple<uint32_t, uint32_t>& maxDimensions ) noexcept;

/// @return 0 - error, task id - otherwise
uint32_t LoadImageAsync( HWND hWnd, const std::wstring& imagePath );

/// @return nullptr - error, pointer to loaded image - otherwise
std::unique_ptr<Gdiplus::Bitmap> LoadImage( const std::wstring& imagePath );

} // namespace smp::image
