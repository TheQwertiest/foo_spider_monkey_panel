#pragma once

#include <mutex>
#include <map>
#include <atomic>
#include <list>


class HostTimer;
class HostTimerTask;


/// @brief Handles JS requests for setInterval, setTimeout, clearInterval, clearTimeout.
/// @details
/// Everything happens inside of the main thread except for:
/// - Timer procs: timer proc is called from a worker thread, but JS callback 
///                handling is given back to main thread through window messaging.
/// - Timer destruction: a separate 'killer' thread handles this.
///
/// Usual workflow is like this (MainThread == MT, WorkerThread == WT, KillerThread == KT):
/// MT:createTimer -> MT:timer.start -> WT:proc(timer) >> window_msg >> MT:task.invoke
///                                          \-> WT:timer.remove -> KT:waitForTimer -> KT:killTimer
class HostTimerDispatcher
{
public:
	~HostTimerDispatcher();

	static HostTimerDispatcher& Get();

	unsigned setInterval(HWND hWnd, unsigned delay, IDispatch* pDisp);
	unsigned setTimeout(HWND hWnd, unsigned delay, IDispatch* pDisp);

	void killTimer(unsigned timerId);

public: // callbacks
	void onPanelUnload(HWND hWnd);

	/// @brief Callback from timer via window message
	/// @details WT:timerProc >> window_msg >> MT:invoke
	void onInvokeMessage(unsigned timerId);

	/// @brief Callback from Killer Thread when timer is expired
	void onTimerExpire(unsigned timerId);

	/// @brief Callback from HostTimer when timer proc finished execution
	void onTimerStopRequest(HWND hWnd, HANDLE hTimer, unsigned timerId);

	/// @brief Callback from HostTimerTask when task has been completed
	void onTaskComplete(unsigned timerId);

private:
	HostTimerDispatcher();

	unsigned createTimer(HWND hWnd, unsigned delay, bool isRepeated, IDispatch* pDisp);

private: //thread
	enum ThreadTaskId
	{
		killTimerTask,
		shutdownTask
	};

	void createThread();
	void stopThread();

	void threadMain();

private:
	typedef std::map<unsigned, std::unique_ptr<HostTimer>> TimerMap;
	typedef std::map<unsigned, std::unique_ptr<HostTimerTask>> TaskMap;	

	std::mutex m_timerMutex;

	HANDLE m_hTimerQueue;
	
	TimerMap m_timerMap;
	TaskMap m_taskMap;

	unsigned m_curTimerId;

private: // thread
	typedef struct
	{
		ThreadTaskId taskId;

		HWND hWnd;
		unsigned timerId;
		HANDLE hTimer;
	} ThreadTask;

	std::thread* m_thread;

	std::mutex m_threadTaskMutex;
	std::list<ThreadTask> m_threadTaskList;

	std::condition_variable m_cv;
};

/// @brief Task that should be executed on timer proc
/// @details Also handles IDispatch references
class HostTimerTask
{
public:
	/// @brief ctor
	/// @details Adds reference to pDisp
	HostTimerTask(IDispatch * pDisp, unsigned timerId);

	/// @brief dtor
	/// @details Removes reference from pDisp
	~HostTimerTask();

	/// @brief Adds reference to task
	void acquire();

	/// @brief Removes reference from task
	/// @details Self-destruct when task reference is zero
	void release();

	/// @brief Invokes JS callback
	/// @details Adds task reference on enter and removes on exit,
	///          so if the corresponding timer is dead, it will self-destruct.
	void invoke();

private:
	IDispatch * m_pDisp;

	unsigned m_timerId;
	unsigned m_refCount;
};

class HostTimer
{
public:
	HostTimer(HWND hWnd, unsigned id, unsigned delay, bool isRepeated);
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

	IDispatch * m_pDisp;
	HANDLE m_hTimer;

	unsigned m_id;
	unsigned m_delay;
	bool m_isRepeated;

	std::atomic<bool> m_isStopRequested;
	bool m_isStopped;
};
