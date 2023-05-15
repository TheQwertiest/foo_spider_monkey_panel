#pragma once

#include <js_backend/objects/core/object_base.h>
#include <js_backend/utils/js_heap_helper.h>

#include <js/TypeDecls.h>

namespace Gdiplus
{
class Graphics;
class Bitmap;
} // namespace Gdiplus

namespace mozjs
{

class WindowCanvas;
class CanvasRenderingContext2d;

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
    [[nodiscard]] size_t GetInternalSize();

public:
    JSObject* GetContext( const std::wstring& contextType, JS::HandleValue attributes = JS::UndefinedHandleValue );
    JSObject* GetContextWithOpt( size_t optArgCount, const std::wstring& contextType, JS::HandleValue attributes );

    uint32_t get_Height() const;
    uint32_t get_Width() const;

private:
    [[nodiscard]] WindowCanvas( JSContext* cx, Gdiplus::Graphics& graphics, uint32_t width, uint32_t height );

private:
    JSContext* pJsCtx_ = nullptr;
    HeapHelper heapHelper_;

    Gdiplus::Graphics* pGraphics_ = nullptr;
    uint32_t width_ = 0;
    uint32_t height_ = 0;

    std::optional<size_t> jsRenderingContextHeapIdOpt_;
    CanvasRenderingContext2d* pNativeRenderingContext_ = nullptr;
};

} // namespace mozjs
