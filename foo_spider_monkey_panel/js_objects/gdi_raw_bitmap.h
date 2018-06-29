#pragma once

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;

namespace Gdiplus
{
class Bitmap;
}

namespace mozjs
{

class JsGdiRawBitmap
{
public:
    ~JsGdiRawBitmap();
    
    static JSObject* Create( JSContext* cx, Gdiplus::Bitmap* p_bmp );

    static const JSClass& GetClass();

    HDC GetHDC() const;

public: // props
    std::optional<std::uint32_t> get_Height();
    std::optional<std::uint32_t> get_Width();

private:
    JsGdiRawBitmap( JSContext* cx, Gdiplus::Bitmap* p_bmp );
    JsGdiRawBitmap( const JsGdiRawBitmap& ) = delete;

private:
    JSContext * pJsCtx_ = nullptr;;
    HDC hDc_ = nullptr;;
    HBITMAP hBmp_ = nullptr;;
    HBITMAP hBmpOld_ = nullptr;;
    uint32_t width_;
    uint32_t height_;
};

}
