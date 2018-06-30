#pragma once

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;


namespace mozjs
{

class JsFbPlaybackQueueItem
{
public:
    ~JsFbPlaybackQueueItem();

    static JSObject* Create( JSContext* cx, const t_playback_queue_item& playbackQueueItem );

    static const JSClass& GetClass();

public:
    std::optional<JSObject*> get_Handle();
    std::optional<uint32_t> get_PlaylistIndex();
    std::optional<uint32_t> get_PlaylistItemIndex();

private:
    JsFbPlaybackQueueItem( JSContext* cx, const t_playback_queue_item& playbackQueueItem );
    JsFbPlaybackQueueItem( const JsFbPlaybackQueueItem& ) = delete;
    JsFbPlaybackQueueItem& operator=( const JsFbPlaybackQueueItem& ) = delete;

private:
    JSContext * pJsCtx_ = nullptr;
    t_playback_queue_item playbackQueueItem_;
};

}
