#pragma once

namespace smp
{

class Runnable
{
public:
    virtual ~Runnable() = default;
    /// @remark Must be executed on main thread only
    virtual void Run() = 0;
};

} // namespace smp
