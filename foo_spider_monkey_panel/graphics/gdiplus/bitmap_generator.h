#pragma once

struct IWICBitmap;

namespace smp::graphics
{

/// @throw qwr::QwrException
std::unique_ptr<Gdiplus::Bitmap> GenerateGdiBitmap( IWICBitmap& wicBitmap );

} // namespace smp::graphics
