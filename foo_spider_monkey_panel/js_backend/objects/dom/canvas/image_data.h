#pragma once

#include <js_backend/objects/core/object_base.h>

#include <js/TypeDecls.h>

namespace mozjs
{

class ImageData;

template <>
struct JsObjectTraits<ImageData>
{
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = true;

    static const JSClass JsClass;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
    static const JSNative JsConstructor;
};

class ImageData
    : public JsObjectBase<ImageData>
{
public:
    ~ImageData() override;

    [[nodiscard]] static std::unique_ptr<ImageData> CreateNative( JSContext* cx, uint32_t width, uint32_t height, JS::HandleObject pixels );
    [[nodiscard]] size_t GetInternalSize() const;
    [[nodiscard]] std::vector<uint8_t> GetDataCopy() const;

    static void Trace( JSTracer* trc, JSObject* obj );

    uint32_t GetHeight() const;
    uint32_t GetWidth() const;

public:
    static JSObject* Constructor( JSContext* cx, uint32_t width, uint32_t height, JS::HandleValue dataArray );

    JSObject* get_Data() const;

    uint32_t get_Height() const;
    uint32_t get_Width() const;

private:
    [[nodiscard]] ImageData( JSContext* cx, uint32_t width, uint32_t height, JS::HandleObject pixels );

private:
    JSContext* pJsCtx_ = nullptr;

    const uint32_t width_;
    const uint32_t height_;

    JS::Heap<JSObject*> pixels_;
};

} // namespace mozjs
