#pragma once

#include <js_backend/objects/core/object_base.h>

#include <js/TypeDecls.h>

namespace mozjs
{

class PlaceHolder;

template <>
struct JsObjectTraits<PlaceHolder>
{
    static constexpr bool HasProto = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
};

class PlaceHolder
    : public JsObjectBase<PlaceHolder>
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( PlaceHolder );

public:
    ~PlaceHolder() override;

    [[nodiscard]] static std::unique_ptr<PlaceHolder> CreateNative( JSContext* cx );
    [[nodiscard]] size_t GetInternalSize() const;

public:
    void Dummy();

private:
    [[nodiscard]] PlaceHolder( JSContext* cx );

private:
    JSContext* pJsCtx_ = nullptr;
};

} // namespace mozjs
