#pragma once

#include <js_backend/objects/dom/event.h>

namespace smp
{

class PlaybackStopEvent;

}

namespace mozjs
{

class PlaybackStopEvent
    : public JsObjectBase<PlaybackStopEvent>
    , private JsEvent
{
    friend class JsObjectBase<PlaybackStopEvent>;

private:
    static constexpr bool HasProto = false;
    static constexpr bool HasParentProto = true;

    using BaseJsType = JsEvent;

    static const JSClass JsClass;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId BasePrototypeId;
    static const JsPrototypeId ParentPrototypeId;

public:
    ~PlaybackStopEvent() override = default;

public:
    uint8_t get_Reason();

private:
    PlaybackStopEvent( JSContext* cx, play_control::t_stop_reason stopReason );

    static std::unique_ptr<PlaybackStopEvent> CreateNative( JSContext* cx, const smp::PlaybackStopEvent& event );
    [[nodiscard]] size_t GetInternalSize();

private:
    JSContext* pJsCtx_ = nullptr;

    play_control::t_stop_reason stopReason_ = play_control::stop_reason_user;
};

} // namespace mozjs
