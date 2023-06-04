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
    static const JSFunctionSpec* JsFunctions;
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

public:
    JSObject* CreateImageBitmap1( JS::HandleValue image, JS::HandleValue options = JS::UndefinedHandleValue );
    JSObject* CreateImageBitmap2( JS::HandleValue image,
                                  JS::HandleValue sxValue, int32_t sy,
                                  int32_t sw, int32_t sh,
                                  JS::HandleValue options = JS::UndefinedHandleValue );
    JSObject* CreateImageBitmapWithOpt( size_t optArgCount, JS::HandleValue image,
                                        JS::HandleValue arg1, int32_t arg2,
                                        int32_t arg3, int32_t arg4,
                                        JS::HandleValue arg5 );

protected:
    [[nodiscard]] ModuleCanvas( JSContext* cx );

private:
    JSContext* pJsCtx_ = nullptr;
};

} // namespace mozjs
