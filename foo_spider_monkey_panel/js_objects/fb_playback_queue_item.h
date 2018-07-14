#pragma once

#include <js_objects/object_base.h>

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
    static constexpr bool HasProxy = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;

public:
    ~JsFbPlaybackQueueItem();

    static std::unique_ptr<JsFbPlaybackQueueItem> CreateNative( JSContext* cx, const t_playback_queue_item& playbackQueueItem );
    static size_t GetInternalSize( const t_playback_queue_item& playbackQueueItem );

public:
    std::optional<JSObject*> get_Handle();
    std::optional<uint32_t> get_PlaylistIndex();
    std::optional<uint32_t> get_PlaylistItemIndex();

private:
    JsFbPlaybackQueueItem( JSContext* cx, const t_playback_queue_item& playbackQueueItem );

private:
    JSContext * pJsCtx_ = nullptr;
    t_playback_queue_item playbackQueueItem_;
};

}
