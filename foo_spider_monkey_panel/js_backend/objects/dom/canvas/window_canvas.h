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

class WindowCanvas;
class CanvasRenderingContext2D_Qwr;

template <>
struct JsObjectTraits<WindowCanvas>
{
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
};

class WindowCanvas
    : public JsObjectBase<WindowCanvas>
{
public:
    ~WindowCanvas() override;

    [[nodiscard]] static std::unique_ptr<WindowCanvas> CreateNative( JSContext* cx, Gdiplus::Graphics& graphics, uint32_t width, uint32_t height );
    [[nodiscard]] size_t GetInternalSize() const;

public:
    JSObject* GetContext( JS::HandleObject jsSelf, const std::wstring& contextType, JS::HandleValue attributes = JS::UndefinedHandleValue );
    JSObject* GetContextWithOpt( JS::HandleObject jsSelf, size_t optArgCount, const std::wstring& contextType, JS::HandleValue attributes );

    uint32_t get_Height() const;
    uint32_t get_Width() const;

private:
    [[nodiscard]] WindowCanvas( JSContext* cx, Gdiplus::Graphics& graphics, uint32_t width, uint32_t height );

private:
    JSContext* pJsCtx_ = nullptr;

    Gdiplus::Graphics* pGraphics_ = nullptr;
    uint32_t width_ = 0;
    uint32_t height_ = 0;

    CanvasRenderingContext2D_Qwr* pNativeRenderingContext_ = nullptr;
};

} // namespace mozjs
