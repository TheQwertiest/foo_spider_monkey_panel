#pragma once

#include <js_backend/engine/js_event_status.h>
#include <js_backend/objects/core/object_base.h>
#include <js_backend/objects/dom/event_target.h>
#include <tasks/events/event.h>

#include <js/TypeDecls.h>

namespace mozjs
{

class Process;

template <>
struct JsObjectTraits<Process>
{
    using ParentJsType = JsEventTarget;

    static constexpr bool HasProto = false;
    static constexpr bool HasParentProto = true;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
};

class Process final
    : public JsObjectBase<Process>
    , private JsEventTarget
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( Process );

public:
    static const std::unordered_set<smp::EventId> kHandledEvents;

public:
    ~Process() override;

    static std::unique_ptr<Process> CreateNative( JSContext* cx );
    [[nodiscard]] size_t GetInternalSize() const;

    static void Trace( JSTracer* trc, JSObject* obj );

    EventStatus HandleEvent( JS::HandleObject self, const smp::EventBase& event ) override;

public:
    JSObject* CpuUsage( JS::HandleValue previousValue = JS::UndefinedHandleValue ) const;
    JSObject* CpuUsageWithOpt( size_t optArgCount, JS::HandleValue previousValue ) const;
    std::wstring Cwd() const;
    void Exit() const;
    // TODO: move memory usage to SMP module?
    JSObject* MemoryUsage() const;

    // TODO: add env
    std::wstring get_ExecPath() const;
    JS::BigInt* get_HrTime() const;
    JSObject* get_LaunchOptions() const;
    std::wstring get_ProfilePath() const;
    qwr::u8string get_Version() const;

    // TODO: add component handling

private:
    [[nodiscard]] Process( JSContext* cx );

    [[nodiscard]] const std::string& EventIdToType( smp::EventId eventId );

private:
    JSContext* pJsCtx_ = nullptr;

    mutable JS::Heap<JSObject*> jsLaunchOptions_;
};

} // namespace mozjs
