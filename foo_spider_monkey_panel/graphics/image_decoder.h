#pragma once

#include <qwr/com_ptr.h>

struct IWICBitmap;

namespace smp
{

struct LoadedImage;

/// @throw qwr::QwrException
qwr::ComPtr<IWICBitmap> DecodeImage( const LoadedImage& loadedImage );

} // namespace smp
