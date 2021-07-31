#pragma once

#include <timeout/time_types.h>

#include <chrono>
#include <memory>

namespace mozjs
{

class JsAsyncTask;

}

namespace smp
{

class Timeout;

class Timeout
{
public:
    Timeout( uint32_t id, TimeDuration interval, bool isRepeated, std::shared_ptr<mozjs::JsAsyncTask> task );
    ~Timeout() = default;

    void SetWhen( const TimeStamp& baseTime, const TimeDuration& delay );
    void SetFiringId( uint32_t firingId );
    void DisableRepeat();
    void MarkAsStopped();
    void SetRunningState( bool isRunning );

    [[nodiscard]] uint32_t Id() const;
    [[nodiscard]] std::shared_ptr<mozjs::JsAsyncTask> Task() const;
    [[nodiscard]] const TimeStamp& When() const;
    [[nodiscard]] bool IsRunning() const;
    [[nodiscard]] bool IsRepeated() const;
    [[nodiscard]] TimeDuration Interval() const;
    [[nodiscard]] uint32_t GetFiringId();

private:
    const uint32_t id_;
    const TimeDuration interval_;
    const std::shared_ptr<mozjs::JsAsyncTask> task_;
    bool isRepeated_;

    TimeStamp executeAt_;

    bool isRunning_ = false;
    bool isStopped_ = false;

    uint32_t firingId_ = 0;
};

} // namespace smp
