#pragma once

#include <js_backend/utils/heap_data_holder.h>
#include <tasks/events/panel_event.h>
#include <utils/not_null.h>

#include <js/TypeDecls.h>

namespace smp
{

class JsRunnableEvent
    : public PanelEvent

{
public:
    [[nodiscard]] JsRunnableEvent( mozjs::HeapDataHolder_Object heapHolder );

    /// @remark This should be called only from valid js ctx
    [[nodiscard]] JSObject* GetJsTarget();

    /// @throw qwr::QwrException
    /// @throw smp::JsException
    virtual void RunJs() = 0;

private:
    const qwr::u8string type_;
    JSContext* pJsCtx_ = nullptr;

    mozjs::HeapDataHolder_Object heapHolder_;
};

} // namespace smp
