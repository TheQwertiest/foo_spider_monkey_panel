#pragma once

#include <js_backend/engine/js_event_status.h>
#include <js_backend/objects/core/object_base.h>
#include <js_backend/objects/dom/event_target.h>
#include <tasks/events/event.h>

#include <optional>
#include <string>
#include <unordered_set>

namespace mozjs
{

class PlaybackControl;

template <>
struct JsObjectTraits<PlaybackControl>
{
    using ParentJsType = JsEventTarget;

    static constexpr bool HasProto = false;
    static constexpr bool HasParentProto = true;
    static constexpr bool HasPostCreate = true;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const PostJsCreateFn PostCreate;
};

class PlaybackControl final
    : public JsObjectBase<PlaybackControl>
    , private JsEventTarget
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( PlaybackControl );

public:
    static const std::unordered_set<smp::EventId> kHandledEvents;

public:
    ~PlaybackControl() override = default;

    static std::unique_ptr<PlaybackControl> CreateNative( JSContext* cx );
    [[nodiscard]] size_t GetInternalSize() const;
    static void PostCreate( JSContext* cx, JS::HandleObject self );

    static void Trace( JSTracer* trc, JSObject* obj );

    EventStatus HandleEvent( JS::HandleObject self, const smp::EventBase& event ) override;

public:
    JSObject* GetCurrentlyPlayingTrack();
    void Next();
    void Pause();
    void Play();
    void Previous();
    void PlayRandom();
    void Stop();
    void VolumeDown();
    void VolumeMute();
    void VolumeUp();

public:
    bool get_IsPaused() const;
    bool get_IsPlaying() const;
    double get_CurrentTime() const;
    qwr::u8string get_PlaybackOrder() const;
    bool get_StopAfterCurrent() const;
    float get_Volume() const;
    void put_CurrentTime( double time );
    void put_PlaybackOrder( const qwr::u8string& value );
    void put_StopAfterCurrent( bool value );
    void put_Volume( float value );

private:
    [[nodiscard]] PlaybackControl( JSContext* cx );

    [[nodiscard]] const std::string& EventIdToType( smp::EventId eventId );

private:
    JSContext* pJsCtx_ = nullptr;
};

} // namespace mozjs
