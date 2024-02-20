#pragma once

#include <js_backend/objects/core/object_base.h>

#include <js/TypeDecls.h>

namespace mozjs
{

class ComponentManager;

template <>
struct JsObjectTraits<ComponentManager>
{
    static constexpr bool HasProto = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
};

class ComponentManager
    : public JsObjectBase<ComponentManager>
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( ComponentManager );

public:
    ~ComponentManager() override;

    [[nodiscard]] static std::unique_ptr<ComponentManager> CreateNative( JSContext* cx );
    [[nodiscard]] size_t GetInternalSize() const;

public:
    JSObject* FindComponentByFilename( const qwr::u8string& filename ) const;
    JSObject* FindComponentByName( const qwr::u8string& name ) const;

private:
    JSObject* CreateJsComponent( componentversion::ptr pComponent ) const;

private:
    [[nodiscard]] ComponentManager( JSContext* cx );

private:
    JSContext* pJsCtx_ = nullptr;
};

} // namespace mozjs
