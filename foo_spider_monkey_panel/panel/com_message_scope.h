#pragma once

namespace smp
{

// Should be used to avoid deadlocks caused by re-entrance in message loop (e.g. during JS object finalization).
// COM filters out most messages before loop re-entrance, but some (most notably WM_PAINT) are passed through.
class ComMessageScope
{
public:
    ComMessageScope();
    ~ComMessageScope();

    static bool IsInScope();

private:
    static uint32_t scopeRefs_;
};

} // namespace smp
