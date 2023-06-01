#pragma once

#include <js_backend/objects/core/object_base.h>

namespace mozjs
{

class ModuleCanvas;

template <>
struct JsObjectTraits<ModuleCanvas>
{
    static constexpr bool HasProto = false;
    static constexpr bool HasParentProto = false;
    static constexpr bool HasPostCreate = true;

    static const JSClass JsClass;
    static const PostJsCreateFn PostCreate;
};

class ModuleCanvas
    : public JsObjectBase<ModuleCanvas>
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( ModuleCanvas );

public:
    ~ModuleCanvas() override = default;

    static std::unique_ptr<ModuleCanvas> CreateNative( JSContext* cx );
    [[nodiscard]] size_t GetInternalSize();
    static void PostCreate( JSContext* cx, JS::HandleObject self );

protected:
    [[nodiscard]] ModuleCanvas( JSContext* cx );

private:
    JSContext* pJsCtx_ = nullptr;
};

} // namespace mozjs
