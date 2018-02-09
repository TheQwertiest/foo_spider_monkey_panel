#include "stdafx.h"
#include "host_timer_dispatcher.h"
#include "user_message.h"

HostTimerDispatcher::HostTimerDispatcher() 
{
	m_curTimerId = 1;
	m_hTimerQueue = CreateTimerQueue();
}

HostTimerDispatcher::~HostTimerDispatcher()
{
	stopThread();
	// Clear all references
	m_taskMap.clear();
}

HostTimerDispatcher& HostTimerDispatcher::Get()
{
	static HostTimerDispatcher timerDispatcher;
	return timerDispatcher;
}

unsigned HostTimerDispatcher::setInterval(HWND hWnd, unsigned delay, IDispatch* pDisp)
{
	return createTimer(hWnd, delay, true, pDisp);
}

unsigned HostTimerDispatcher::setTimeout(HWND hWnd, unsigned delay, IDispatch* pDisp)
{
	return createTimer(hWnd, delay, false, pDisp);
}

void HostTimerDispatcher::killTimer(unsigned timerId)
{
	{
		std::lock_guard<std::mutex> lock(m_timerMutex);

		auto timerIter = m_timerMap.find(timerId);
		if (m_timerMap.end() != timerIter)
		{
			timerIter->second->stop();
		}		
	}

	auto taskIter = m_taskMap.find(timerId);
	if (m_taskMap.end() != taskIter)
	{
		taskIter->second->release();
	}
}

void HostTimerDispatcher::onPanelUnload(HWND hWnd)
{
	std::list<unsigned> timersToDelete;

	{
		std::lock_guard<std::mutex> lock(m_timerMutex);
		for each (const auto& elem in m_timerMap)
		{
			if (elem.second->GetHwnd() == hWnd)
			{
				timersToDelete.push_back(elem.first);
			}
		}
	}

	for each (auto timerId in timersToDelete)
	{
		killTimer(timerId);
	}
}

void HostTimerDispatcher::onInvokeMessage(unsigned timerId)
{
	if (m_taskMap.end() != m_taskMap.find(timerId))
	{
		m_taskMap[timerId]->invoke();
	}
}

void HostTimerDispatcher::onTimerExpire(unsigned timerId)
{
	std::unique_lock<std::mutex> lock(m_timerMutex);

	m_timerMap.erase(timerId);
}

void HostTimerDispatcher::onTimerStopRequest(HWND hWnd, HANDLE hTimer, unsigned timerId)
{
	std::unique_lock<std::mutex> lock(m_threadTaskMutex);

	ThreadTask threadTask = {};
	threadTask.taskId = killTimerTask;
	threadTask.hWnd = hWnd;
	threadTask.hTimer = hTimer;
	threadTask.timerId = timerId;

	m_threadTaskList.push_front(threadTask);
	m_cv.notify_one();
}

void HostTimerDispatcher::onTaskComplete(unsigned timerId)
{
	if (m_taskMap.end() != m_taskMap.find(timerId))
	{
		m_taskMap.erase(timerId);
	}
}

unsigned HostTimerDispatcher::createTimer(HWND hWnd, unsigned delay, bool isRepeated, IDispatch* pDisp)
{
	if (!pDisp)
	{
		return 0;
	}

	std::lock_guard<std::mutex> lock(m_timerMutex);

	unsigned id = m_curTimerId++;
	while (m_taskMap.end() != m_taskMap.find(id) && m_timerMap.end() != m_timerMap.find(id))
	{
		id = m_curTimerId++;
	}

	m_timerMap.emplace(id, new HostTimer(hWnd, id, delay, isRepeated));

	auto & curTask = m_taskMap.emplace(id, new HostTimerTask(pDisp, id));
	curTask.first->second->acquire();

	if (!m_timerMap[id]->start(m_hTimerQueue))
	{
		m_timerMap.erase(id);
		m_taskMap.erase(id);
		return 0;
	}

	return id;
}

void HostTimerDispatcher::createThread()
{
	m_thread = new std::thread(&HostTimerDispatcher::threadMain, this);
}

void HostTimerDispatcher::stopThread()
{
	if (!m_thread)
	{
		return;
	}

	{
		std::lock_guard<std::mutex> lock(m_threadTaskMutex);
		ThreadTask threadTask = {};
		threadTask.taskId = shutdownTask;

		m_threadTaskList.push_front(threadTask);
		m_cv.notify_one();
	}

	if (m_thread->joinable())
	{
		m_thread->join();
	}

	delete m_thread;
	m_thread = NULL;
}

void HostTimerDispatcher::threadMain()
{
	while (true)
	{
		ThreadTask threadTask;
		{
			std::unique_lock<std::mutex> lock(m_threadTaskMutex);

			while (m_threadTaskList.empty())
			{
				m_cv.wait(lock);
			}

			if (m_threadTaskList.empty())
			{
				continue;
			}

			threadTask = m_threadTaskList.front();
			m_threadTaskList.pop_front();
		}

		switch (threadTask.taskId)
		{
		case killTimerTask:
		{
			DeleteTimerQueueTimer(m_hTimerQueue, threadTask.hTimer, INVALID_HANDLE_VALUE);
			onTimerExpire(threadTask.timerId);
			
			break;
		}
		case shutdownTask:
		{
			DeleteTimerQueueEx(m_hTimerQueue, INVALID_HANDLE_VALUE);
			m_hTimerQueue = NULL;

			return;
		}
		default:
		{
			assert(0);
			break;
		}
		}
	}
}

HostTimerTask::HostTimerTask(IDispatch * pDisp, unsigned timerId)
{
	m_pDisp = pDisp;
	m_timerId = timerId;

	m_refCount = 0;
	m_pDisp->AddRef();
}

HostTimerTask::~HostTimerTask()
{
	m_pDisp->Release();
}

void HostTimerTask::acquire()
{
	++m_refCount;
}

void HostTimerTask::release()
{
	if (!m_refCount)
	{
		return;
	}

	--m_refCount;
	if (!m_refCount)
	{
		HostTimerDispatcher::Get().onTaskComplete(m_timerId);
	}
}

void HostTimerTask::invoke()
{
	acquire();

	VARIANTARG args[1];
	args[0].vt = VT_I4;
	args[0].lVal = m_timerId;
	DISPPARAMS dispParams = { args, NULL, _countof(args), 0 };
	m_pDisp->Invoke(DISPID_VALUE, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispParams, NULL, NULL, NULL);

	release();
}

HostTimer::HostTimer(HWND hWnd, unsigned id, unsigned delay, bool isRepeated)
{
	m_hTimer = 0;

	m_hWnd = hWnd;
	m_delay = delay;
	m_isRepeated = isRepeated;
	m_id = id;

	m_isStopRequested = false;
	m_isStopped = false;
}

HostTimer::~HostTimer()
{

}

bool HostTimer::start(HANDLE hTimerQueue)
{
	return !!CreateTimerQueueTimer(
		&m_hTimer,
		hTimerQueue,
		HostTimer::timerProc,
		this,
		m_delay,
		m_isRepeated ? m_delay : 0,
		WT_EXECUTEINTIMERTHREAD | (m_isRepeated ? 0 : WT_EXECUTEONLYONCE));
}

void HostTimer::stop()
{
	m_isStopRequested = true;
}

VOID CALLBACK HostTimer::timerProc(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
	HostTimer* timer = (HostTimer*)lpParameter;

	if (timer->m_isStopped)
	{
		return;
	}

	if (timer->m_isStopRequested)
	{
		timer->m_isStopped = true;
		HostTimerDispatcher::Get().onTimerStopRequest(timer->m_hWnd, timer->m_hTimer, timer->m_id);

		return;
	}

	if (!timer->m_isRepeated)
	{
		timer->m_isStopped = true;
		SendMessage(timer->m_hWnd, UWM_TIMER, timer->m_id, 0);
		HostTimerDispatcher::Get().onTimerStopRequest(timer->m_hWnd, timer->m_hTimer, timer->m_id);

		return;
	}

	SendMessage(timer->m_hWnd, UWM_TIMER, timer->m_id, 0);
}

HWND HostTimer::GetHwnd() const
{
	return m_hWnd;
}

HANDLE HostTimer::GetHandle() const
{
	return m_hTimer;
}
