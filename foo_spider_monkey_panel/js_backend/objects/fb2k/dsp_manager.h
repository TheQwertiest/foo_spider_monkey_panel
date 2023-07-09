#pragma once

#include <js_backend/objects/core/object_base.h>
#include <js_backend/objects/dom/event_target.h>
#include <tasks/events/event_id.h>

#include <js/TypeDecls.h>

namespace mozjs
{

class DspManager;

template <>
struct JsObjectTraits<DspManager>
{
    using ParentJsType = JsEventTarget;

    static constexpr bool HasProto = false;
    static constexpr bool HasParentProto = true;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
};

class DspManager
    : public JsObjectBase<DspManager>
    , private JsEventTarget
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( DspManager );

public:
    static const std::unordered_set<smp::EventId> kHandledEvents;

public:
    ~DspManager() override;

    [[nodiscard]] static std::unique_ptr<DspManager> CreateNative( JSContext* cx );
    [[nodiscard]] size_t GetInternalSize() const;

    static void Trace( JSTracer* trc, JSObject* obj );

    EventStatus HandleEvent( JS::HandleObject self, const smp::EventBase& event ) override;

    JS::Value GetItem( uint32_t index ) const;

public:
    void ActivateChainPreset( uint32_t index );
    JS::Value GetActiveChainPreset() const;
    // TODO: add index argument to edit non-active chain (if actually possible)
    void ShowConfigure() const;
    JS::Value GetChainPresets() const;

private:
    [[nodiscard]] DspManager( JSContext* cx );

    [[nodiscard]] const std::string& EventIdToType( smp::EventId eventId );

private:
    JSContext* pJsCtx_ = nullptr;
};

} // namespace mozjs
