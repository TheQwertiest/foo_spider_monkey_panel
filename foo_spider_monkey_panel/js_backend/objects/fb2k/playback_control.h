#pragma once

#include <events/event.h>
#include <js_backend/objects/core/object_base.h>
#include <js_backend/objects/dom/event_target.h>

#include <optional>
#include <string>
#include <unordered_set>

namespace mozjs
{

class PlaybackControl
    : public JsObjectBase<PlaybackControl>
    , private JsEventTarget
{
    friend class JsObjectBase<PlaybackControl>;

private:
    static constexpr bool HasProto = false;
    static constexpr bool HasParentProto = true;
    static constexpr bool HasPostCreate = true;

    using BaseJsType = JsEventTarget;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId BasePrototypeId;
    static const JsPrototypeId ParentPrototypeId;

public:
    static std::unordered_set<smp::EventId> kHandledEvents;

public:
    ~PlaybackControl() override = default;

    static std::unique_ptr<PlaybackControl> CreateNative( JSContext* cx );

    static void Trace( JSTracer* trc, JSObject* obj );
    void PrepareForGc();

    void HandleEvent( JS::HandleObject self, const smp::EventBase& event );

public:
    JSObject* GetNowPlayingTrack();
    void Next();
    void Pause();
    void Play();
    void Prev();
    void Random();
    void Stop();
    void VolumeDown();
    void VolumeMute();
    void VolumeUp();

public:
    bool get_IsPaused();
    bool get_IsPlaying();
    double get_CurrentTime();
    bool get_StopAfterCurrent();
    float get_Volume();
    void put_CurrentTime( double time );
    void put_StopAfterCurrent( bool p );
    void put_Volume( float value );

private:
    PlaybackControl( JSContext* cx );

    [[nodiscard]] size_t GetInternalSize();
    static void PostCreate( JSContext* cx, JS::HandleObject self );

    const std::string& EventIdToType( smp::EventId eventId );

private:
    JSContext* pJsCtx_ = nullptr;
};

} // namespace mozjs
