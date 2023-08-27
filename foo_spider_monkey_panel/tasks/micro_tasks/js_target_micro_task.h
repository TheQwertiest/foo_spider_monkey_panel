#pragma once

#include <js_backend/utils/js_heap_helper.h>
#include <tasks/cancellable.h>
#include <tasks/micro_tasks/micro_task_runnable.h>

#include <js/TypeDecls.h>

namespace smp
{

class JsTargetMicroTask
    : public MicroTaskRunnable
    , public Cancellable
{
public:
    using RunnerFn = std::function<void( JSContext*, JS::HandleObject )>;

public:
    JsTargetMicroTask( JSContext* pJsCtx, JS::HandleObject jsTarget, RunnerFn fn );

    void Run() final;

private:
    JSContext* pJsCtx_ = nullptr;

    mozjs::HeapHelper heapHelper_;
    const uint32_t jsTargetId_;

    const RunnerFn fn_;
};

} // namespace smp
