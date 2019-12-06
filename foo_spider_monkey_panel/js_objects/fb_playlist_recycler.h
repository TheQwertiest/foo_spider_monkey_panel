#pragma once

#include <js_objects/object_base.h>

#include <optional>
#include <string>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsFbPlaylistRecycler
    : public JsObjectBase<JsFbPlaylistRecycler>
{
public:
    static constexpr bool HasProto = false;
    static constexpr bool HasProxy = false;
    static constexpr bool HasPostCreate = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;

public:
    ~JsFbPlaylistRecycler() override = default;

    static std::unique_ptr<JsFbPlaylistRecycler> CreateNative( JSContext* cx );
    static size_t GetInternalSize();

public:
    JSObject* GetContent( uint32_t index );
    pfc::string8_fast GetName( uint32_t index );
    void Purge( JS::HandleValue affectedItems );
    void Restore( uint32_t index );

public:
    uint32_t get_Count();

private:
    JsFbPlaylistRecycler( JSContext* cx );

private:
    JSContext* pJsCtx_ = nullptr;
};

} // namespace mozjs
