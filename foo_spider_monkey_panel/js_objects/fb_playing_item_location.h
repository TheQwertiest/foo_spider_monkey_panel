#pragma once

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsFbPlayingItemLocation
{
public:
    ~JsFbPlayingItemLocation();

    static JSObject* Create( JSContext* cx, bool isValid, uint32_t playlistIndex, uint32_t playlistItemIndex );

    static const JSClass& GetClass();

public:
    std::optional<bool> get_IsValid();
    std::optional<uint32_t> get_PlaylistIndex();
    std::optional<uint32_t> get_PlaylistItemIndex();

private:
    JsFbPlayingItemLocation( JSContext* cx, bool isValid, uint32_t playlistIndex, uint32_t playlistItemIndex );
    JsFbPlayingItemLocation( const JsFbPlayingItemLocation& ) = delete;
    JsFbPlayingItemLocation& operator=( const JsFbPlayingItemLocation& ) = delete;

private:
    JSContext * pJsCtx_ = nullptr;
    bool isValid_;
    uint32_t playlistIndex_;
    uint32_t playlistItemIndex_;
};

}
