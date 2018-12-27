#pragma once

namespace smp
{

// Should be used to avoid deadlocks caused by re-entrance in message loop (e.g. in during JS object finalization)
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
