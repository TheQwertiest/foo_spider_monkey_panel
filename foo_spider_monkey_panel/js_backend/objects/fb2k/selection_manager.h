#pragma once

#include <events/event.h>
#include <js_backend/engine/js_event_status.h>
#include <js_backend/objects/core/object_base.h>
#include <js_backend/objects/dom/event_target.h>

#include <optional>
#include <string>
#include <unordered_set>

namespace mozjs
{

class JsFbMetadbHandleList;

class SelectionManager;

template <>
struct JsObjectTraits<SelectionManager>
{
    using ParentJsType = JsEventTarget;

    static constexpr bool HasProto = false;
    static constexpr bool HasParentProto = true;
    static constexpr bool HasPostCreate = true;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const PostJsCreateFn PostCreate;
};

class SelectionManager
    : public JsObjectBase<SelectionManager>
    , private JsEventTarget
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( SelectionManager );

public:
    static const std::unordered_set<smp::EventId> kHandledEvents;

public:
    ~SelectionManager() override = default;

    static std::unique_ptr<SelectionManager> CreateNative( JSContext* cx );
    [[nodiscard]] size_t GetInternalSize();
    static void PostCreate( JSContext* cx, JS::HandleObject self );

    static void Trace( JSTracer* trc, JSObject* obj );
    void PrepareForGc();

    EventStatus HandleEvent( JS::HandleObject self, const smp::EventBase& event );

public:
    // TODO: revisit ui_selection_holder interactions after implementing on_focus replacements
    JSObject* GetSelection( uint32_t flags = 0 );
    JSObject* GetSelectionWithOpt( size_t optArgCount, uint32_t flags );
    GUID GetSelectionType();
    void SetPlaylistSelectionTracking();
    void SetPlaylistTracking();
    void SetSelection( JsFbMetadbHandleList* handles, const GUID& type = {} );
    void SetSelectionWithOpt( size_t optArgCount, JsFbMetadbHandleList* handles, const GUID& type );

private:
    [[nodiscard]] SelectionManager( JSContext* cx );

    [[nodiscard]] const std::string& EventIdToType( smp::EventId eventId );

private:
    JSContext* pJsCtx_ = nullptr;
    ui_selection_holder::ptr holder_;
};

} // namespace mozjs
