#pragma once

#include <js_backend/utils/js_heap_helper.h>
#include <tasks/events/panel_event.h>
#include <utils/not_null.h>

#include <js/TypeDecls.h>

namespace smp
{

class JsRunnableEvent
    : public PanelEvent

{
public:
    [[nodiscard]] JsRunnableEvent( JSContext* pJsCtx, JS::HandleObject jsTarget );
    [[nodiscard]] JsRunnableEvent( JSContext* pJsCtx, const std::shared_ptr<mozjs::HeapHelper>& pHeapHelper, uint32_t jsTargetId );

    [[nodiscard]] JSObject* GetJsTarget();

    /// @throw qwr::QwrException
    /// @throw smp::JsException
    virtual void RunJs() = 0;

private:
    const qwr::u8string type_;
    JSContext* pJsCtx_ = nullptr;

    smp::not_null<std::shared_ptr<mozjs::HeapHelper>> pHeapHelper_;
    const uint32_t jsTargetId_;
};

} // namespace smp
