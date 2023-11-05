#pragma once

#include <js_backend/objects/core/object_base.h>
#include <js_backend/objects/dom/event_target.h>
#include <tasks/events/event_id.h>

#include <js/TypeDecls.h>

namespace mozjs
{

class OutputManager;

template <>
struct JsObjectTraits<OutputManager>
{
    using ParentJsType = JsEventTarget;

    static constexpr bool HasProto = false;
    static constexpr bool HasParentProto = true;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
};

class OutputManager
    : public JsObjectBase<OutputManager>
    , private JsEventTarget
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( OutputManager );

public:
    static const std::unordered_set<smp::EventId> kHandledEvents;

public:
    ~OutputManager() override;

    [[nodiscard]] static std::unique_ptr<OutputManager> CreateNative( JSContext* cx );
    [[nodiscard]] size_t GetInternalSize() const;

    static void Trace( JSTracer* trc, JSObject* obj );

    EventStatus HandleEvent( JS::HandleObject self, const smp::EventBase& event ) override;

public:
    JSObject* GetActiveDeviceOutput() const;
    JS::Value GetDeviceOutputs() const;
    // TODO: add options for dither and bitdepth
    void SetActiveDeviceOutput( const std::wstring& outputId, const std::wstring& deviceId );

private:
    [[nodiscard]] OutputManager( JSContext* cx );

    [[nodiscard]] const std::string& EventIdToType( smp::EventId eventId ) const;

private:
    JSContext* pJsCtx_ = nullptr;
};

} // namespace mozjs
