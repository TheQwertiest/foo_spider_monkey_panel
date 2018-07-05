#pragma once

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsMenuObject;
class JsFbMetadbHandleList;

class JsContextMenuManager
{
public:
    ~JsContextMenuManager();

    static JSObject* Create( JSContext* cx );

    static const JSClass& GetClass();

public:
    std::optional<std::nullptr_t> BuildMenu( JsMenuObject* menuObject, int32_t base_id, int32_t max_id = -1 );
    std::optional<std::nullptr_t> BuildMenuWithOpt( size_t optArgCount, JsMenuObject* menuObject, int32_t base_id, int32_t max_id );
    std::optional<bool> ExecuteByID( uint32_t id );
    std::optional<std::nullptr_t> InitContext( JsFbMetadbHandleList* handles );
    std::optional<std::nullptr_t> InitNowPlaying();

private:
    JsContextMenuManager( JSContext* cx );
    JsContextMenuManager( const JsContextMenuManager& ) = delete;
    JsContextMenuManager& operator=( const JsContextMenuManager& ) = delete;

private:
    JSContext * pJsCtx_ = nullptr;
    contextmenu_manager::ptr contextMenu_;
};

}
