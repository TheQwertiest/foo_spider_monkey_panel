#pragma once

#include <qwr/com_ptr.h>

namespace smp::graphics
{

struct LoadedImage
{
    size_t width;
    size_t height;
    qwr::ComPtr<IStream> pDataStream;
};

} // namespace smp::graphics
