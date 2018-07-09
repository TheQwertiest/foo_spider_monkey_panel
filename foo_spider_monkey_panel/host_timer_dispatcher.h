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

	/// @brief Callback from Killer Thread when timer is expired
	void onTimerExpire( uint32_t timerId);

	/// @brief Callback from HostTimer when timer proc finished execution
	void onTimerStopRequest(HWND hWnd, HANDLE hTimer, uint32_t timerId);

	/// @brief Callback from HostTimerTask when task has been completed
	void onTaskComplete( uint32_t timerId);

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
	typedef std::map<unsigned, std::unique_ptr<HostTimer>> TimerMap;
    // TODO: may be replace unique_ptr with shared_ptr (also check std::enable_shared_from_this)
	typedef std::map<unsigned, std::unique_ptr<HostTimerTask>> TaskMap;

	HANDLE m_hTimerQueue;
	std::mutex m_timerMutex;
	TaskMap m_taskMap;
	TimerMap m_timerMap;
    uint32_t m_curTimerId;

private: // thread
	typedef struct
	{
		ThreadTaskId taskId;
		HWND hWnd;
        uint32_t timerId;
		HANDLE hTimer;
	} ThreadTask;

	std::thread* m_thread;
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
	HostTimerTask( uint32_t timerId, JSContext* cx, JS::HandleFunction func );

	/// @brief dtor
	~HostTimerTask();

	/// @brief Adds reference to task
	void acquire();

	/// @brief Removes reference from task
	/// @details When task reference is zero removes func from heap and self-destructs
	void release();

	/// @brief Invokes JS callback
	/// @details Adds task reference on enter and removes on exit,
	///          so if the corresponding timer is dead, it will self-destruct.
	void invoke();

private:
    JSContext * pJsCtx_ = nullptr;

    uint32_t funcId_;
    uint32_t globalId_;
    mozjs::JsGlobalObject* pNativeGlobal_ = nullptr;

    uint32_t m_timerId;
    uint32_t m_refCount = 0;
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
	HWND m_hWnd;

	HANDLE m_hTimer;

    uint32_t m_id;
    uint32_t m_delay;
	bool m_isRepeated;

	std::atomic<bool> m_isStopRequested;
	bool m_isStopped;
};
