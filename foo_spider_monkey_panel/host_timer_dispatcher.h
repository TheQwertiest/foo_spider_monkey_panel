#pragma once

#include <mutex>
#include <map>
#include <atomic>
#include <list>

namespace mozjs
{
class JsGlobalObject;
}

class HostTimer;
class HostTimerTask;

/*
@brief Handles JS requests for setInterval, setTimeout, clearInterval, clearTimeout.
@details
Everything happens inside of the main thread except for:
- Timer procs: timer proc is called from a worker thread, but JS callback
  handling is given back to main thread through window messaging.
- Timer destruction: a separate 'killer' thread handles this.

Usual workflow is like this (MainThread == MT, WorkerThread == WT, KillerThread == KT):
 MT:createTimer -> MT:timer.start -> WT:proc(timer) >> window_msg >> MT:task.invoke
                                       \-> WT:timer.remove -> KT:waitForTimer -> KT:killTimer
*/

class HostTimerDispatcher
{
public:
	~HostTimerDispatcher();

	static HostTimerDispatcher& Get();

    uint32_t setInterval( HWND hWnd, uint32_t delay, JSContext* cx, JS::HandleFunction jsFunction );
    uint32_t setTimeout( HWND hWnd, uint32_t delay, JSContext* cx, JS::HandleFunction jsFunction );

	void killTimer( uint32_t timerId);

public: // callbacks
	void onPanelUnload(HWND hWnd);

	/// @brief Callback from timer via window message
	/// @details WT:timerProc >> window_msg >> MT:invoke
	void onInvokeMessage( uint32_t timerId);

	/// @brief Callback from KT: when timer is expired
	void onTimerExpire( uint32_t timerId);

	/// @brief Callback from WT: from HostTimer when timer proc finished execution
	void onTimerStopRequest(HWND hWnd, HANDLE hTimer, uint32_t timerId);

private:
	HostTimerDispatcher();

	unsigned createTimer( HWND hWnd, uint32_t delay, bool isRepeated, JSContext* cx, JS::HandleFunction jsFunction );

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
    struct TimerObject
    {
        TimerObject( std::unique_ptr<HostTimer> timerArg, std::unique_ptr<HostTimerTask> taskArg )
            : timer(std::move( timerArg ) )
            , task( std::move( taskArg ) )
        {
        }
        std::unique_ptr<HostTimer> timer;
        std::shared_ptr<HostTimerTask> task;
    };
    using TimerMap = std::map<uint32_t, std::unique_ptr<TimerObject>>;

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

	std::thread* m_thread = nullptr;
	std::mutex m_threadTaskMutex;
	std::list<ThreadTask> m_threadTaskList;
	std::condition_variable m_cv;
};

/// @brief Task that should be executed on timer proc
/// @details Everything apart from destructor is performed on the MainThread
class HostTimerTask
{
public:
	/// @brief ctor
    /// @details Pushes func to heap	
	HostTimerTask( JSContext* cx, JS::HandleFunction func );

	/// @brief dtor
	~HostTimerTask();

	/// @brief Invokes JS callback
	void invoke();

    void DisableHeapCleanup();

private:
    JSContext * pJsCtx_ = nullptr;

    uint32_t funcId_;
    uint32_t globalId_;
    mozjs::JsGlobalObject* pNativeGlobal_ = nullptr;

    bool needsCleanup_ = false;
};

class HostTimer
{
public:
	HostTimer(HWND hWnd, uint32_t id, uint32_t delay, bool isRepeated);
	~HostTimer();

	bool start(HANDLE hTimerQueue);
	void stop();

	/// @brief Timer proc.
	/// @details Short execution, since it only sends window message to MT:invoke.
	///          If it's a timeout timer, requests self-removal from killer thread.
	///
	/// @param[in] lpParameter Pointer to HostTimer object
	static VOID CALLBACK timerProc(PVOID lpParameter, BOOLEAN TimerOrWaitFired);

	HWND GetHwnd() const;
	HANDLE GetHandle() const;

private:
	HWND m_hWnd = nullptr;
	HANDLE m_hTimer = nullptr;

    uint32_t m_id;
    uint32_t m_delay;
	bool m_isRepeated;

	std::atomic<bool> m_isStopRequested = false;
	bool m_isStopped = false;
};
