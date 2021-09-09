#pragma once

#include <optional>
#include <string>
#include <tuple>

namespace smp::image
{

[[nodiscard]] std::tuple<uint32_t, uint32_t>
GetResizedImageSize( const std::tuple<uint32_t, uint32_t>& currentDimension,
                     const std::tuple<uint32_t, uint32_t>& maxDimensions ) noexcept;

/// @return 0 - error, task id - otherwise
[[nodiscard]] uint32_t LoadImageAsync( HWND hWnd, const std::wstring& imagePath );

/// @return nullptr - error, pointer to loaded image - otherwise
[[nodiscard]] std::unique_ptr<Gdiplus::Bitmap>
LoadImage( const std::wstring& imagePath );

/// @remark WIC loads images 'eagerly' which makes loading operation slower by x100, so it should be used only as a last resort
[[nodiscard]] std::unique_ptr<Gdiplus::Bitmap>
LoadImageWithWIC( IStreamPtr pStream );

} // namespace smp::image
