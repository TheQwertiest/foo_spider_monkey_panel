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

    uint32_t setInterval( HWND hWnd, uint32_t delay, JSContext* cx, JS::HandleFunction jsFunction, JS::HandleValueArray jsFuncArgs );
    uint32_t setTimeout( HWND hWnd, uint32_t delay, JSContext* cx, JS::HandleFunction jsFunction, JS::HandleValueArray jsFuncArgs );

    void killTimer( uint32_t timerId );

public: // callbacks
    void onPanelUnload( HWND hWnd );

    /// @brief Callback from KT: when timer is expired
    void onTimerExpire( uint32_t timerId );

    /// @brief Callback from WT: from HostTimer when timer proc finished execution
    void onTimerStopRequest( HWND hWnd, HANDLE hTimer, uint32_t timerId );

private:
    HostTimerDispatcher();

    /// @throw smp::JsException
    uint32_t createTimer( HWND hWnd, uint32_t delay, bool isRepeated, JSContext* cx, JS::HandleFunction jsFunction, JS::HandleValueArray jsFuncArgs );

private: //thread
    enum class ThreadTaskId
    {
        killTimerTask,
        shutdownTask
    };

    void createThread();
    void stopThread();

    void threadMain();

private:
    using TimerMap = std::map<uint32_t, std::unique_ptr<HostTimer>>;

    HANDLE m_hTimerQueue = nullptr;
    std::mutex m_timerMutex;
    TimerMap m_timerMap;
    uint32_t m_curTimerId;

private: // thread
    struct ThreadTask
    {
        ThreadTaskId taskId;
        HWND hWnd;
        uint32_t timerId;
        HANDLE hTimer;
    };

    std::unique_ptr<std::thread> m_thread;
    std::mutex m_threadTaskMutex;
    std::list<ThreadTask> m_threadTaskList;
    std::condition_variable m_cv;
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
    /// throws JsException
    bool InvokeJsImpl( JSContext* cx, JS::HandleObject jsGlobal, JS::HandleValue funcValue, JS::HandleValue argArrayValue ) override;
};

class HostTimer
{
public:
    HostTimer( HWND hWnd, uint32_t id, uint32_t delay, bool isRepeated, std::shared_ptr<HostTimerTask> task );
    ~HostTimer() = default;

    bool start( HANDLE hTimerQueue );
    void stop();

    /// @brief Timer proc.
    /// @details Delegates task execution to the main thread via window message.
    ///          If it's a timeout timer, requests self-removal from killer thread.
    ///
    /// @param[in] lpParameter Pointer to HostTimer object
    static VOID CALLBACK timerProc( PVOID lpParameter, BOOLEAN TimerOrWaitFired );

    HWND GetHwnd() const;
    HostTimerTask& GetTask();

private:
    std::shared_ptr<HostTimerTask> task_;

    HWND hWnd_ = nullptr;
    HANDLE hTimer_ = nullptr;

    uint32_t id_;
    uint32_t delay_;
    bool isRepeated_;

    std::atomic_bool isStopRequested_ = false;
    bool isStopped_ = false;
};
