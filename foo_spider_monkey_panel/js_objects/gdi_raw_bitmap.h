#pragma once

#include <js_objects/object_base.h>

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
    : public JsObjectBase<JsGdiRawBitmap>
{
public:
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = false;
    static constexpr bool HasProxy = false;
    static constexpr bool HasPostCreate = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;

public:
    ~JsGdiRawBitmap();

    static std::unique_ptr<JsGdiRawBitmap> CreateNative( JSContext* cx, Gdiplus::Bitmap* pBmp );
    static size_t GetInternalSize( Gdiplus::Bitmap* pBmp );

public:
    HDC GetHDC() const;

public: // props
    std::optional<std::uint32_t> get_Height();
    std::optional<std::uint32_t> get_Width();

private:
    JsGdiRawBitmap( JSContext* cx, HDC hDc, HBITMAP hBmp, uint32_t width, uint32_t height );

private:
    JSContext * pJsCtx_ = nullptr;
    HDC hDc_ = nullptr;
    HBITMAP hBmp_ = nullptr;
    HBITMAP hBmpOld_ = nullptr;
    uint32_t width_;
    uint32_t height_;
};

}
