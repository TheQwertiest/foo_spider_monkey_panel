#pragma once

namespace smp
{

class Cancellable
{
public:
    ~Cancellable() = default;

    [[nodiscard]] bool IsCancelled() const;
    void Cancel();

private:
    bool isCanceled_ = false;
};

} // namespace smp
