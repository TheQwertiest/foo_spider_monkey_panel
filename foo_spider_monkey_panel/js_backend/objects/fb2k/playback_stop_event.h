#pragma once

#include <js_backend/objects/dom/event.h>

namespace mozjs
{

class PlaybackStopEvent
    : public JsObjectBase<PlaybackStopEvent>
    , private JsEvent
{
    friend class JsObjectBase<PlaybackStopEvent>;

private:
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = true;
    static constexpr bool HasParentProto = true;

    using BaseJsType = JsEvent;

    static const JSClass JsClass;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId BasePrototypeId;
    static const JsPrototypeId ParentPrototypeId;
    static const JsPrototypeId PrototypeId;
    static const JSNative JsConstructor;

protected:
    struct EventOptions
    {
        BaseJsType::EventOptions baseOptions;
        play_control::t_stop_reason reason = play_control::t_stop_reason::stop_reason_user;
    };

public:
    ~PlaybackStopEvent() override = default;

public:
    static JSObject* Constructor( JSContext* cx, const qwr::u8string& type, JS::HandleValue options = JS::UndefinedHandleValue );
    static JSObject* ConstructorWithOpt( JSContext* cx, size_t optArgCount, const qwr::u8string& type, JS::HandleValue options );

    uint8_t get_Reason() const;

protected:
    PlaybackStopEvent( JSContext* cx, const qwr::u8string& type, const EventOptions& options = {} );
    [[nodiscard]] size_t GetInternalSize();

    static EventOptions ExtractOptions( JSContext* cx, JS::HandleValue options );

private:
    static std::unique_ptr<PlaybackStopEvent> CreateNative( JSContext* cx, const qwr::u8string& type, play_control::t_stop_reason reason );
    static std::unique_ptr<PlaybackStopEvent> CreateNative( JSContext* cx, const qwr::u8string& type, const EventOptions& options );

private:
    JSContext* pJsCtx_ = nullptr;

    play_control::t_stop_reason stopReason_ = play_control::stop_reason_user;
};

} // namespace mozjs
