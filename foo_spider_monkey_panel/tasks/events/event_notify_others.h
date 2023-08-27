#pragma once

#include <js_backend/utils/js_heap_helper.h>
#include <tasks/events/event.h>
#include <tasks/events/event_js_executor.h>

namespace smp
{

class Event_NotifyOthers final
    : public Event_JsExecutor
{
public:
    Event_NotifyOthers( JSContext* cx, const std::wstring& name, JS::HandleValue info );
    ~Event_NotifyOthers() override;

    std::unique_ptr<EventBase> Clone() override;

    std::optional<bool> JsExecute( mozjs::JsContainer& jsContainer ) override;

private:
    JSContext* pJsCtx_ = nullptr;

    mozjs::HeapHelper heapHelper_;

    const std::wstring name_;
    const uint32_t jsInfoId_;
};

} // namespace smp
