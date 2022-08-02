#pragma once

#include <js_backend/objects/core/object_base.h>

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

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;

public:
    ~JsFbPlayingItemLocation() override = default;

    static std::unique_ptr<JsFbPlayingItemLocation> CreateNative( JSContext* cx, bool isValid, uint32_t playlistIndex, uint32_t playlistItemIndex );
    [[nodiscard]] size_t GetInternalSize();

public:
    bool get_IsValid();
    uint32_t get_PlaylistIndex();
    uint32_t get_PlaylistItemIndex();

private:
    JsFbPlayingItemLocation( JSContext* cx, bool isValid, uint32_t playlistIndex, uint32_t playlistItemIndex );

private:
    [[maybe_unused]] JSContext* pJsCtx_ = nullptr;
    bool isValid_;
    uint32_t playlistIndex_;
    uint32_t playlistItemIndex_;
};

} // namespace mozjs
