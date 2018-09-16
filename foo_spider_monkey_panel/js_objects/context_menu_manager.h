#pragma once

#include <js_objects/object_base.h>

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsMenuObject;
class JsFbMetadbHandleList;

class JsContextMenuManager
    : public JsObjectBase<JsContextMenuManager>
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
    ~JsContextMenuManager();

    static std::unique_ptr<JsContextMenuManager> CreateNative( JSContext* cx );
    static size_t GetInternalSize();

public:
    std::optional<std::nullptr_t> BuildMenu( JsMenuObject* menuObject, int32_t base_id, int32_t max_id = -1 );
    std::optional<std::nullptr_t> BuildMenuWithOpt( size_t optArgCount, JsMenuObject* menuObject, int32_t base_id, int32_t max_id );
    std::optional<bool> ExecuteByID( uint32_t id );
    std::optional<std::nullptr_t> InitContext( JsFbMetadbHandleList* handles );
    std::optional<std::nullptr_t> InitContextPlaylist();
    std::optional<std::nullptr_t> InitNowPlaying();

private:
    JsContextMenuManager( JSContext* cx );

private:
    JSContext * pJsCtx_ = nullptr;
    contextmenu_manager::ptr contextMenu_;
};

}
