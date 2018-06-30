#pragma once

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsPlayingItemLocation
{
public:
    ~JsPlayingItemLocation();

    static JSObject* Create( JSContext* cx, bool isValid, uint32_t playlistIndex, uint32_t playlistItemIndex );

    static const JSClass& GetClass();

public:
    std::optional<bool> get_IsValid();
    std::optional<uint32_t> get_PlaylistIndex();
    std::optional<uint32_t> get_PlaylistItemIndex();

private:
    JsPlayingItemLocation( JSContext* cx, bool isValid, uint32_t playlistIndex, uint32_t playlistItemIndex );
    JsPlayingItemLocation( const JsPlayingItemLocation& ) = delete;

private:
    JSContext * pJsCtx_ = nullptr;
    bool isValid_;
    uint32_t playlistIndex_;
    uint32_t playlistItemIndex_;
};

}
