#pragma once

#include <js_objects/object_base.h>

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsFbPlayingItemLocation
    : public JsObjectBase<JsFbPlayingItemLocation>
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
    ~JsFbPlayingItemLocation();

    static std::unique_ptr<JsFbPlayingItemLocation> CreateNative( JSContext* cx, bool isValid, uint32_t playlistIndex, uint32_t playlistItemIndex );
    static size_t GetInternalSize( bool isValid, uint32_t playlistIndex, uint32_t playlistItemIndex );

public:
    std::optional<bool> get_IsValid();
    std::optional<uint32_t> get_PlaylistIndex();
    std::optional<uint32_t> get_PlaylistItemIndex();

private:
    JsFbPlayingItemLocation( JSContext* cx, bool isValid, uint32_t playlistIndex, uint32_t playlistItemIndex );

private:
    JSContext * pJsCtx_ = nullptr;
    bool isValid_;
    uint32_t playlistIndex_;
    uint32_t playlistItemIndex_;
};

}
