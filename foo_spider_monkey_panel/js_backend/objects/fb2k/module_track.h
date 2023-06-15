#pragma once

#include <js_backend/objects/core/object_base.h>

namespace mozjs
{

class ModuleTrack;

template <>
struct JsObjectTraits<ModuleTrack>
{
    static constexpr bool HasProto = false;
    static constexpr bool HasParentProto = false;
    static constexpr bool HasPostCreate = true;

    static const JSClass JsClass;
    static const PostJsCreateFn PostCreate;
};

class ModuleTrack
    : public JsObjectBase<ModuleTrack>
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( ModuleTrack );

public:
    ~ModuleTrack() override = default;

    static std::unique_ptr<ModuleTrack> CreateNative( JSContext* cx );
    [[nodiscard]] size_t GetInternalSize();
    static void PostCreate( JSContext* cx, JS::HandleObject self );

protected:
    [[nodiscard]] ModuleTrack( JSContext* cx );

private:
    JSContext* pJsCtx_ = nullptr;
};

} // namespace mozjs
