#pragma once

#include <js_backend/utils/js_heap_helper.h>
#include <tasks/cancellable.h>
#include <tasks/runnable.h>

#include <js/TypeDecls.h>

namespace smp
{

class MicroTask
    : public Runnable
    , public Cancellable
{
public:
    using RunnerFn = std::function<void( JSContext*, JS::HandleObject )>;

public:
    MicroTask( JSContext* pJsCtx, JS::HandleObject jsTarget, RunnerFn fn );

public:
    /// @remark Requires Realm initialized with JsTarget to run
    void Run() final;

    JSObject* GetJsTarget();

    bool IsCallable() const;

private:
    JSContext* pJsCtx_ = nullptr;

    mozjs::HeapHelper heapHelper_;
    const uint32_t jsTargetId_;

    const RunnerFn fn_;
};

} // namespace smp
