#pragma once

namespace mozjs
{

// TODO: do we actually need this?
class ICanvasSurface
{

public:
    [[nodiscard]] virtual bool IsDevice() const = 0;
    [[nodiscard]] virtual Gdiplus::Graphics& GetGraphics() = 0;

    [[nodiscard]] virtual Gdiplus::Bitmap* GetBmp()
    {
        return nullptr;
    }

    [[nodiscard]] virtual uint32_t GetHeight() const = 0;
    [[nodiscard]] virtual uint32_t GetWidth() const = 0;
};

} // namespace mozjs
