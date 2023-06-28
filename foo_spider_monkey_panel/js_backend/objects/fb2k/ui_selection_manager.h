#pragma once

#include <js_backend/engine/js_event_status.h>
#include <js_backend/objects/core/object_base.h>
#include <js_backend/objects/dom/event_target.h>
#include <panel/panel_fwd.h>
#include <tasks/events/event.h>

namespace mozjs
{

class UiSelectionManager;

template <>
struct JsObjectTraits<UiSelectionManager>
{
    using ParentJsType = JsEventTarget;

    static constexpr bool HasProto = false;
    static constexpr bool HasParentProto = true;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
};

class UiSelectionManager
    : public JsObjectBase<UiSelectionManager>
    , private JsEventTarget
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( UiSelectionManager );

public:
    static const std::unordered_set<smp::EventId> kHandledEvents;

public:
    ~UiSelectionManager() override = default;

    static std::unique_ptr<UiSelectionManager> CreateNative( JSContext* cx );
    [[nodiscard]] size_t GetInternalSize();

    static void Trace( JSTracer* trc, JSObject* obj );

    EventStatus HandleEvent( JS::HandleObject self, const smp::EventBase& event ) override;

public:
    JSObject* GetSelection( bool ignoreCurrentlyPlaying = true );
    JSObject* GetSelectionWithOpt( size_t optArgCount, bool ignoreCurrentlyPlaying );
    const qwr::u8string& GetSelectionSource( bool ignoreCurrentlyPlaying = true );
    const qwr::u8string& GetSelectionSourceWithOpt( size_t optArgCount, bool ignoreCurrentlyPlaying );
    void SetSelection( JS::HandleValue tracks, const qwr::u8string& type = "undefined" );
    void SetSelectionWithOpt( size_t optArgCount, JS::HandleValue tracks, const qwr::u8string& type );

    void TrackActivePlaylist();
    void TrackActivePlaylistSelection();

private:
    [[nodiscard]] UiSelectionManager( JSContext* cx );

    [[nodiscard]] const std::string& EventIdToType( smp::EventId eventId );

private:
    JSContext* pJsCtx_ = nullptr;
    smp::not_null_shared<smp::panel::PanelAccessor> pHostPanel_;
};

} // namespace mozjs
