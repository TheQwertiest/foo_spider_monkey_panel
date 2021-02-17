#pragma once

#include <js_objects/object_base.h>

#include <optional>
#include <string>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsMenuObject;

class JsMainMenuManager
    : public JsObjectBase<JsMainMenuManager>
{
public:
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = false;
    static constexpr bool HasProxy = false;
    static constexpr bool HasPostCreate = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;

public:
    ~JsMainMenuManager() override = default;

    static std::unique_ptr<JsMainMenuManager> CreateNative( JSContext* cx );
    static size_t GetInternalSize();

public:
    void BuildMenu( JsMenuObject* menu, int32_t base_id, int32_t count );
    bool ExecuteByID( uint32_t id );
    void Init( const qwr::u8string& root_name );

private:
    JsMainMenuManager( JSContext* cx );

private:
    [[maybe_unused]] JSContext* pJsCtx_ = nullptr;
    mainmenu_manager::ptr menuManager_;
};

} // namespace mozjs
