#pragma once

#include <js_backend/objects/core/object_base.h>

#include <js/TypeDecls.h>

namespace Gdiplus
{
class Graphics;
class Bitmap;
} // namespace Gdiplus

namespace mozjs
{

class Canvas;
class CanvasRenderingContext2D_Qwr;

template <>
struct JsObjectTraits<Canvas>
{
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = true;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
    static const JSNative JsConstructor;
};

class Canvas
    : public JsObjectBase<Canvas>
{
public:
    ~Canvas() override;

    [[nodiscard]] static std::unique_ptr<Canvas> CreateNative( JSContext* cx, int32_t width, int32_t height );
    [[nodiscard]] size_t GetInternalSize() const;

public:
    static JSObject* Constructor( JSContext* cx, int32_t width, int32_t height );

    JSObject* GetContext( JS::HandleObject jsSelf, const std::wstring& contextType, JS::HandleValue attributes = JS::UndefinedHandleValue );
    JSObject* GetContextWithOpt( JS::HandleObject jsSelf, size_t optArgCount, const std::wstring& contextType, JS::HandleValue attributes );

    int32_t get_Height() const;
    int32_t get_Width() const;
    void put_Height( int32_t value );
    void put_Width( int32_t value );

private:
    [[nodiscard]] Canvas( JSContext* cx, int32_t width, int32_t height );

    void ReinitializeCanvas( int32_t width, int32_t height );

private:
    JSContext* pJsCtx_ = nullptr;

    std::unique_ptr<Gdiplus::Bitmap> pBitmap_;
    std::unique_ptr<Gdiplus::Graphics> pGraphics_;

    CanvasRenderingContext2D_Qwr* pNativeRenderingContext_ = nullptr;
};

} // namespace mozjs
