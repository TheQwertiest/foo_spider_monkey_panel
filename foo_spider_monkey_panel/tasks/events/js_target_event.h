#pragma once

#include <js_backend/utils/js_heap_helper.h>
#include <tasks/events/panel_event.h>
#include <utils/not_null.h>

#include <js/TypeDecls.h>

namespace smp
{

// TODO: think of a better name to avoid confusion with JsEventTarget
class JsTargetEvent
    : public PanelEvent

{
public:
    [[nodiscard]] JsTargetEvent( const qwr::u8string& type, JSContext* pJsCtx, JS::HandleObject jsTarget );
    [[nodiscard]] JsTargetEvent( const qwr::u8string& type, JSContext* pJsCtx, const std::shared_ptr<mozjs::HeapHelper>& pHeapHelper, uint32_t jsTargetId );

    [[nodiscard]] const qwr::u8string& GetType() const;

public:
    [[nodiscard]] JSObject* GetJsTarget();

private:
    const qwr::u8string type_;
    JSContext* pJsCtx_ = nullptr;

    smp::not_null<std::shared_ptr<mozjs::HeapHelper>> pHeapHelper_;
    const uint32_t jsTargetId_;
};

} // namespace smp
