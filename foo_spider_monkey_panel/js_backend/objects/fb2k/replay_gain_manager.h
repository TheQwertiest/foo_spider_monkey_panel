#pragma once

#include <js_backend/engine/js_event_status.h>
#include <js_backend/objects/core/object_base.h>
#include <js_backend/objects/dom/event_target.h>
#include <tasks/events/event.h>

#include <js/TypeDecls.h>

namespace mozjs
{

class ReplayGainManager;

template <>
struct JsObjectTraits<ReplayGainManager>
{
    using ParentJsType = JsEventTarget;

    static constexpr bool HasProto = false;
    static constexpr bool HasParentProto = true;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
};

class ReplayGainManager
    : public JsObjectBase<ReplayGainManager>
    , private JsEventTarget
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( ReplayGainManager );

public:
    static const std::unordered_set<smp::EventId> kHandledEvents;

public:
    ~ReplayGainManager() override;

    [[nodiscard]] static std::unique_ptr<ReplayGainManager> CreateNative( JSContext* cx );
    [[nodiscard]] size_t GetInternalSize() const;

    static void Trace( JSTracer* trc, JSObject* obj );

    EventStatus HandleEvent( JS::HandleObject self, const smp::EventBase& event ) override;

public:
    void ShowConfigure();

    // TODO: test if playback restart is needed after settings change
    float get_PreampWithRG() const;
    float get_PreampWithoutRG() const;
    qwr::u8string get_ProcessingMode() const;
    qwr::u8string get_SourceMode() const;
    void put_PreampWithRG( float value );
    void put_PreampWithoutRG( float value );
    void put_ProcessingMode( const qwr::u8string& value );
    void put_SourceMode( const qwr::u8string& value );

private:
    [[nodiscard]] ReplayGainManager( JSContext* cx );

    [[nodiscard]] const std::string& EventIdToType( smp::EventId eventId );

private:
    JSContext* pJsCtx_ = nullptr;
};

} // namespace mozjs
