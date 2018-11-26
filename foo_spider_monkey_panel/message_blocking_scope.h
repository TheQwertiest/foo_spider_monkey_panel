#pragma once

namespace smp
{

class MessageBlockingScope
{
public:
    MessageBlockingScope();
    ~MessageBlockingScope();

    static bool IsBlocking();

private:
    static bool isBlocking_;
};

} // namespace smp
