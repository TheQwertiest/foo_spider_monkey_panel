#pragma once

#include <optional>
#include <string>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsFbPlaylistRecyclerManager
{
public:
    ~JsFbPlaylistRecyclerManager();

    static JSObject* Create( JSContext* cx );

    static const JSClass& GetClass();

public:
    //std::optional<std::nullptr_t> Purge( JS::HandleValue affectedItems );
    std::optional<std::nullptr_t> Restore( uint32_t index );

public:
    std::optional<JSObject*> get_Content( uint32_t index );
    std::optional<uint32_t> get_Count();
    std::optional<std::string> get_Name( uint32_t index );

private:
    JsFbPlaylistRecyclerManager( JSContext* cx );
    JsFbPlaylistRecyclerManager( const JsFbPlaylistRecyclerManager& ) = delete;
    JsFbPlaylistRecyclerManager& operator=( const JsFbPlaylistRecyclerManager& ) = delete;

private:
    JSContext * pJsCtx_ = nullptr;
};

}
