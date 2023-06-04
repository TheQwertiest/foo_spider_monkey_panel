#pragma once

#include <graphics/loaded_image.h>

#include <qwr/com_ptr.h>

#include <filesystem>

struct IWICBitmap;

namespace smp::graphics
{

/// @throw qwr::QwrException
std::unique_ptr<const LoadedImage> LoadImageFromFile( const std::filesystem::path& path );

/// @throw qwr::QwrException
qwr::ComPtr<IWICBitmap> DecodeImage( const LoadedImage& loadedImage );

} // namespace smp::graphics
