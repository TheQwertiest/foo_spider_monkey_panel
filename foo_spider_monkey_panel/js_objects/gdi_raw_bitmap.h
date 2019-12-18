#pragma once

#include <js_objects/object_base.h>
#include <utils/gdi_helpers.h>

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
    ~JsGdiRawBitmap() override = default;

    static std::unique_ptr<JsGdiRawBitmap> CreateNative( JSContext* cx, Gdiplus::Bitmap* pBmp );
    static size_t GetInternalSize( Gdiplus::Bitmap* pBmp );

public:
    [[nodiscard]] __notnull
        HDC
        GetHDC() const;

public: // props
    std::uint32_t get_Height();
    std::uint32_t get_Width();

private:
    JsGdiRawBitmap( JSContext* cx,
                    smp::gdi::unique_gdi_ptr<HDC> hDc,
                    smp::gdi::unique_gdi_ptr<HBITMAP> hBmp,
                    uint32_t width,
                    uint32_t height );

private:
    [[maybe_unused]] JSContext* pJsCtx_ = nullptr;
    const smp::gdi::unique_gdi_ptr<HDC> pDc_;
    const smp::gdi::unique_gdi_ptr<HBITMAP> hBmp_;
    const smp::gdi::ObjectSelector<HBITMAP> autoBmp_;
    uint32_t width_;
    uint32_t height_;
};

} // namespace mozjs
