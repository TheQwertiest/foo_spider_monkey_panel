#pragma once

#include <js_backend/objects/core/object_base.h>

#include <optional>
#include <string>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class PlaylistRecycleBin;

template <>
struct JsObjectTraits<PlaylistRecycleBin>
{
    static constexpr bool HasProto = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
};

class PlaylistRecycleBin
    : public JsObjectBase<PlaylistRecycleBin>
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( PlaylistRecycleBin );

public:
    ~PlaylistRecycleBin() override = default;

    static std::unique_ptr<PlaylistRecycleBin> CreateNative( JSContext* cx );
    [[nodiscard]] size_t GetInternalSize();

public:
    JSObject* GetContent( uint32_t index );
    pfc::string8_fast GetName( uint32_t index );
    void Purge( const std::vector<uint32_t>& indices );
    void Restore( uint32_t index );

public:
    uint32_t get_Length();

private:
    PlaylistRecycleBin( JSContext* cx );

private:
    JSContext* pJsCtx_ = nullptr;
};

} // namespace mozjs
