#pragma once

#include <optional>
#include <string>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsMenuObject;

class JsMainMenuManager
{
public:
    ~JsMainMenuManager();

    static JSObject* Create( JSContext* cx );

    static const JSClass& GetClass();

public:
    std::optional<std::nullptr_t> BuildMenu( JsMenuObject* p, int32_t base_id, int32_t count );
    std::optional<bool> ExecuteByID( uint32_t id );
    std::optional<std::nullptr_t> Init( std::string root_name );

private:
    JsMainMenuManager( JSContext* cx );
    JsMainMenuManager( const JsMainMenuManager& ) = delete;
    JsMainMenuManager& operator=( const JsMainMenuManager& ) = delete;

private:
    JSContext * pJsCtx_ = nullptr;
    mainmenu_manager::ptr menuManager_;
};

}
