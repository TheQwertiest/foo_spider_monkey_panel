#pragma once

#include <atomic>

namespace smp
{

class MessageBlockingScope
{
public:
    MessageBlockingScope();
    ~MessageBlockingScope();

    static bool IsBlocking();

private:
    static std::atomic<bool> isBlocking_;
};

} // namespace smp
