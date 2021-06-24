#pragma once

#include <js_utils/js_async_task.h>

#include <atomic>
#include <list>
#include <map>
#include <mutex>

namespace mozjs
{
class JsGlobalObject;
}

namespace smp
{

class HostTimer;
class HostTimerTask;

/// @brief Handles JS requests for setInterval, setTimeout, clearInterval, clearTimeout.
/// @details
/// Everything happens inside of the main thread except for:
/// - Timer procs: timer proc is called from a worker thread, but JS callback
///   handling is given back to main thread through window messaging.
/// - Timer destruction: a separate 'killer' thread handles this.
///
/// Usual workflow is like this (MainThread == MT, WorkerThread == WT, KillerThread == KT):
///  MT:createTimer -> MT:timer.start -> WT:proc(timer) >> window_msg >> MT:panel
///                                        \-> WT:timer.remove -> KT:waitForTimer -> KT:killTimer
class HostTimerDispatcher
{
public:
    ~HostTimerDispatcher();

    static HostTimerDispatcher& Get();

    void Finalize();

    uint32_t SetInterval( HWND hWnd, uint32_t delay, JSContext* cx, JS::HandleFunction jsFunction, JS::HandleValueArray jsFuncArgs );
    uint32_t SetTimeout( HWND hWnd, uint32_t delay, JSContext* cx, JS::HandleFunction jsFunction, JS::HandleValueArray jsFuncArgs );

    void StopTimer( uint32_t timerId );
    void StopTimersForPanel( HWND hWnd );

    /// @brief Called from WT: from HostTimer when timer proc finished execution
    void QueueActualTimerStop( HWND hWnd, HANDLE hTimer, uint32_t timerId );

private:
    HostTimerDispatcher();

    /// @throw smp::JsException
    uint32_t CreateTimer( HWND hWnd, uint32_t delay, bool isRepeated, JSContext* cx, JS::HandleFunction jsFunction, JS::HandleValueArray jsFuncArgs );

    /// @brief Called from KT: when timer is expired
    void EraseTimer( uint32_t timerId );

private: //thread
    enum class ThreadTaskId
    {
        killTimerTask,
        shutdownTask
    };

    void CreateThread();
    void StopThread();

    void ThreadMain();

private:
    using TimerMap = std::map<uint32_t, std::unique_ptr<HostTimer>>;

    HANDLE hTimerQueue_ = nullptr;
    std::mutex timerMutex_;
    TimerMap timerMap_;
    uint32_t curTimerId_ = 1;

private: // thread
    struct ThreadTask
    {
        ThreadTaskId taskId;
        HWND hWnd;
        uint32_t timerId;
        HANDLE hTimer;
    };

    std::unique_ptr<std::thread> thread_;
    std::mutex threadTaskMutex_;
    std::list<ThreadTask> threadTaskList_;
    std::condition_variable cv_;
};

/// @brief Task that should be executed on timer proc
/// @details Everything apart from destructor is performed on the MainThread
class HostTimerTask
    : public mozjs::JsAsyncTaskImpl<JS::HandleValue, JS::HandleValue>
{
public:
    HostTimerTask( JSContext* cx, JS::HandleValue funcValue, JS::HandleValue argArrayValue );
    ~HostTimerTask() override = default;

private:
    /// @throw JsException
    bool InvokeJsImpl( JSContext* cx, JS::HandleObject jsGlobal, JS::HandleValue funcValue, JS::HandleValue argArrayValue ) override;
};

class HostTimer
{
public:
    HostTimer( HWND hWnd, uint32_t id, uint32_t delay, bool isRepeated, std::shared_ptr<HostTimerTask> task );
    ~HostTimer() = default;

    void Start( HANDLE hTimerQueue );
    void Stop();

    /// @brief Timer proc.
    /// @details Delegates task execution to the main thread via window message.
    ///          If it's a timeout timer, requests self-removal from killer thread.
    ///
    /// @param[in] lpParameter Pointer to HostTimer object
    static VOID CALLBACK TimerProc( PVOID lpParameter, BOOLEAN TimerOrWaitFired );

    HWND GetHwnd() const;

private:
    std::shared_ptr<HostTimerTask> task_;

    const HWND hWnd_;
    HANDLE hTimer_ = nullptr;

    const uint32_t id_;
    const uint32_t delay_;
    const bool isRepeated_;

    std::atomic_bool isStopRequested_ = false;
    std::atomic_bool isStopped_ = false;
};

} // namespace smp
