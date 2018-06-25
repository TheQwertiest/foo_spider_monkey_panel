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

/*

STDMETHODIMP get__Handle(HDC* p);

*/

class JsGdiRawBitmap
{
public:
    ~JsGdiRawBitmap();
    
    static JSObject* Create( JSContext* cx, Gdiplus::Bitmap* p_bmp );

    static const JSClass& GetClass();

public: // props
    std::optional<std::uint32_t> Height();
    std::optional<std::uint32_t> Width();

private:
    JsGdiRawBitmap( JSContext* cx, Gdiplus::Bitmap* p_bmp );
    JsGdiRawBitmap( const JsGdiRawBitmap& ) = delete;

private:
    JSContext * pJsCtx_;
    HDC hDc_;
    HBITMAP hBmp_;
    HBITMAP hBmpOld_;
    t_size width_;
    t_size height_;
};

}
