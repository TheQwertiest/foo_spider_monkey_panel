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
    static constexpr bool HasPostCreate = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;

public:
    ~JsFbPlaylistRecyclerManager();

    static std::unique_ptr<JsFbPlaylistRecyclerManager> CreateNative( JSContext* cx );
    static size_t GetInternalSize();

public:
    void Purge( JS::HandleValue affectedItems );
    void Restore( uint32_t index );

public:
    JSObject* get_Content( uint32_t index );
    uint32_t get_Count();
    pfc::string8_fast get_Name( uint32_t index );

private:
    JsFbPlaylistRecyclerManager( JSContext* cx );

private:
    JSContext* pJsCtx_ = nullptr;
};

} // namespace mozjs
