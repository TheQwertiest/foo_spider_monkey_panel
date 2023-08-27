#pragma once

#include <qwr/com_ptr.h>

#include <vector>

namespace smp
{

struct LoadedImage
{
    size_t width;
    size_t height;
    std::vector<uint8_t> rawData;
};

} // namespace smp
