#pragma once

namespace smp
{

class MicroTaskRunnable
{
public:
    MicroTaskRunnable() = default;
    virtual ~MicroTaskRunnable() = default;

    virtual void Run() = 0;
};

} // namespace smp
