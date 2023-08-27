#pragma once

#include <tasks/events/js_runnable_event.h>

namespace smp
{

class JsPromiseEvent final
    : public JsRunnableEvent

{
public:
    [[nodiscard]] JsPromiseEvent( JSContext* pJsCtx, mozjs::HeapDataHolder_Object heapHolder, std::function<JS::Value()> promiseResolver );
    [[nodiscard]] JsPromiseEvent( JSContext* pJsCtx, mozjs::HeapDataHolder_Object heapHolder, std::exception_ptr pException );

    void RunJs() final;

private:
    JSContext* pJsCtx_ = nullptr;
    std::function<JS::Value()> promiseResolver_;
    std::exception_ptr pException_;
};

} // namespace smp
