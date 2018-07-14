#pragma once

#include <js_objects/object_base.h>

#include <optional>
#include <string>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsFbPlaylistRecyclerManager
    : public JsObjectBase<JsFbPlaylistRecyclerManager>
{
public:
    static constexpr bool HasProto = false;
    static constexpr bool HasProxy = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;

public:
    ~JsFbPlaylistRecyclerManager();

    static std::unique_ptr<JsFbPlaylistRecyclerManager> CreateNative( JSContext* cx );
    static size_t GetInternalSize();

public:
    std::optional<std::nullptr_t> Purge( JS::HandleValue affectedItems );
    std::optional<std::nullptr_t> Restore( uint32_t index );

public:
    std::optional<JSObject*> get_Content( uint32_t index );
    std::optional<uint32_t> get_Count();
    std::optional<pfc::string8_fast> get_Name( uint32_t index );

private:
    JsFbPlaylistRecyclerManager( JSContext* cx );

private:
    JSContext * pJsCtx_ = nullptr;
};

}
