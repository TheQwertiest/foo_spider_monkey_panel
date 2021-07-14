#pragma once

#include <js_utils/js_async_task.h>

#include <chrono>
#include <map>
#include <unordered_map>

// TODO: add nesting clamp (<https://searchfox.org/mozilla-central/source/dom/base/TimeoutManager.cpp>)

namespace mozjs
{
class JsGlobalObject;
}

namespace smp
{

class Timeout;
class TimeoutJsTask;

class TimeoutManager
{
public:
    TimeoutManager() = default;
    TimeoutManager();

    void Finalize();

    uint32_t SetInterval( uint32_t delay, JSContext* cx, JS::HandleFunction jsFunction, JS::HandleValueArray jsFuncArgs );
    uint32_t SetTimeout( uint32_t delay, JSContext* cx, JS::HandleFunction jsFunction, JS::HandleValueArray jsFuncArgs );

    void StopTimer( uint32_t timerId );

private:
    std::map<std::chrono::time_point<std::chrono::steady_clock>, uint32_t, std::greater<>> timeoutSchedule_;
    std::unordered_map<uint32_t, std::unique_ptr<Timeout>> timeouts_;

    uint32_t curTimerId_ = 1;
};

class Timeout
{
public:
    Timeout( HWND hWnd, uint32_t id, uint32_t delay, bool isRepeated, std::shared_ptr<TimeoutJsTask> task );
    ~Timeout() = default;

private:
    const HWND hWnd_;

    const uint32_t id_;
    const uint32_t delay_;
    const bool isRepeated_;

    std::shared_ptr<TimeoutJsTask> task_;

    std::chrono::time_point<std::chrono::steady_clock> executeAt_;

    bool isRunning_ = false;
    bool isStopped_ = false;
};

class TimeoutJsTask
    : public mozjs::JsAsyncTaskImpl<JS::HandleValue, JS::HandleValue>
{
public:
    TimeoutJsTask( JSContext* cx, JS::HandleValue funcValue, JS::HandleValue argArrayValue );
    ~TimeoutJsTask() override = default;

private:
    /// @throw JsException
    bool InvokeJsImpl( JSContext* cx, JS::HandleObject jsGlobal, JS::HandleValue funcValue, JS::HandleValue argArrayValue ) override;
};

} // namespace smp
