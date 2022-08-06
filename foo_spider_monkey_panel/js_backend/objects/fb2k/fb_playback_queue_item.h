#pragma once

#include <js_backend/objects/core/object_base.h>

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsFbPlaybackQueueItem
    : public JsObjectBase<JsFbPlaybackQueueItem>
{
public:
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;

public:
    ~JsFbPlaybackQueueItem() override = default;

    static std::unique_ptr<JsFbPlaybackQueueItem> CreateNative( JSContext* cx, const t_playback_queue_item& playbackQueueItem );
    [[nodiscard]] size_t GetInternalSize();

public:
    JSObject* get_Handle();
    uint32_t get_PlaylistIndex();
    uint32_t get_PlaylistItemIndex();

private:
    JsFbPlaybackQueueItem( JSContext* cx, const t_playback_queue_item& playbackQueueItem );

private:
    JSContext* pJsCtx_ = nullptr;
    t_playback_queue_item playbackQueueItem_;
};

} // namespace mozjs
