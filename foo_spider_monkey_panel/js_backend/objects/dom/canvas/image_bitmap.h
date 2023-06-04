#pragma once

#include <js_backend/objects/core/object_base.h>

#include <js/TypeDecls.h>

namespace mozjs
{

class ImageBitmap;

template <>
struct JsObjectTraits<ImageBitmap>
{
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
    static const JSNative JsConstructor;
};

class ImageBitmap
    : public JsObjectBase<ImageBitmap>
{
public:
    struct CreateBitmapOptions
    {
        bool shouldFlipY = false;
        std::optional<uint32_t> resizeWidthOpt;
        std::optional<uint32_t> resizeHeightOpt;
        qwr::u8string resizeQuality = "low";
    };

public:
    ~ImageBitmap() override;

    [[nodiscard]] static std::unique_ptr<ImageBitmap> CreateNative( JSContext* cx, std::unique_ptr<Gdiplus::Image> pImage );
    [[nodiscard]] size_t GetInternalSize() const;

    static JSObject* CreateImageBitmap1( JSContext* cx, JS::HandleValue image, JS::HandleValue options );
    static JSObject* CreateImageBitmap2( JSContext* cx, JS::HandleValue image, int32_t sx, int32_t sy, int32_t sw, int32_t sh, JS::HandleValue options );

    [[nodiscard]] Gdiplus::Image* GetBitmap();

public:
    void Close();

    uint32_t get_Height() const;
    uint32_t get_Width() const;

private:
    [[nodiscard]] ImageBitmap( JSContext* cx, std::unique_ptr<Gdiplus::Image> pImage );

    static JSObject* CreateImageBitmapImpl( JSContext* cx, JS::HandleValue image, int32_t sx, int32_t sy, std::optional<int32_t> sw, std::optional<int32_t> sh, JS::HandleValue options );
    static CreateBitmapOptions ParseCreateBitmapOptions( JSContext* cx, JS::HandleValue options );

private:
    JSContext* pJsCtx_ = nullptr;

    std::unique_ptr<Gdiplus::Image> pImage_;
};

} // namespace mozjs
