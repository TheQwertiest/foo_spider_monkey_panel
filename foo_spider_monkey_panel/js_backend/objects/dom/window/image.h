#pragma once

#include <js_backend/objects/core/object_base.h>

#include <js/TypeDecls.h>

namespace mozjs
{

class Image;

template <>
struct JsObjectTraits<Image>
{
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = true;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
    static const JSNative JsConstructor;
};

class Image
    : public JsObjectBase<Image>
{
public:
    ~Image() override;

    [[nodiscard]] static std::unique_ptr<Image> CreateNative( JSContext* cx );
    [[nodiscard]] size_t GetInternalSize() const;

public:
    static JSObject* Constructor( JSContext* cx );

private:
    [[nodiscard]] Image( JSContext* cx );

private:
    JSContext* pJsCtx_ = nullptr;
};

} // namespace mozjs
