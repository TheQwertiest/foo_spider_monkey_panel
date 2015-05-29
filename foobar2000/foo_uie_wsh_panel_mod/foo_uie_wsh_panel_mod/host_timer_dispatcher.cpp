#include "stdafx.h"
#include "host_timer_dispatcher.h"
#include "user_message.h"


HostTimerDispatcher::HostTimerDispatcher() : m_hWnd(NULL)
{
	TIMECAPS tc;
	if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) == TIMERR_NOERROR)
	{
		m_accuracy = min(max(tc.wPeriodMin, 5), tc.wPeriodMax);
	}

	timeBeginPeriod(m_accuracy);
}

HostTimerDispatcher::~HostTimerDispatcher()
{
	reset();
	timeEndPeriod(m_accuracy);
}

unsigned HostTimerDispatcher::setIntervalLegacy(unsigned delay)
{
	return timeSetEvent(delay, m_accuracy, g_timer_proc_legacy, reinterpret_cast<DWORD_PTR>(m_hWnd), TIME_PERIODIC);
}

unsigned HostTimerDispatcher::setInterval(unsigned delay, IDispatch * pDisp)
{
	if (!pDisp) return 0;
	unsigned timerID = timeSetEvent(delay, m_accuracy, g_timer_proc, reinterpret_cast<DWORD_PTR>(m_hWnd), TIME_PERIODIC);
	addTimerMap(timerID, pDisp);
	return timerID;
}

unsigned HostTimerDispatcher::setTimeoutLegacy(unsigned delay)
{
	return timeSetEvent(delay, m_accuracy, g_timer_proc_legacy, reinterpret_cast<DWORD_PTR>(m_hWnd), TIME_ONESHOT);
}

unsigned HostTimerDispatcher::setTimeout(unsigned delay, IDispatch * pDisp)
{
	if (!pDisp) return 0;
	unsigned timerID = timeSetEvent(delay, m_accuracy, g_timer_proc, reinterpret_cast<DWORD_PTR>(m_hWnd), TIME_ONESHOT);
	addTimerMap(timerID, pDisp);
	return timerID;
}

void HostTimerDispatcher::killLegacy(unsigned timerID)
{
	timeKillEvent(timerID);
}

void HostTimerDispatcher::kill(unsigned timerID)
{
	timeKillEvent(timerID);

	if (m_timerDispatchMap.exists(timerID))
	{
		IDispatch * pDisp = m_timerDispatchMap[timerID];
		if (pDisp) pDisp->Release();
		m_timerDispatchMap.remove(timerID);
	}
}

void HostTimerDispatcher::invoke(UINT timerId)
{
	IDispatch * pDisp = NULL;
	if (!m_timerDispatchMap.query(timerId, pDisp) || !pDisp)
		return;

	VARIANTARG args[1];
	args[0].vt = VT_I4;
	args[0].lVal = timerId;
	DISPPARAMS dispParams = { args, NULL, _countof(args), 0 };
	pDisp->Invoke(DISPID_VALUE, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispParams, NULL, NULL, NULL);
}

void HostTimerDispatcher::reset()
{
	for (auto iter = m_timerDispatchMap.first(); iter.is_valid(); iter++)
	{
		timeKillEvent(iter->m_key);
		IDispatch * pDisp = iter->m_value;
		if (pDisp) pDisp->Release();
	}

	m_timerDispatchMap.remove_all();
}

void HostTimerDispatcher::addTimerMap(unsigned timerID, IDispatch * pDisp)
{
	PFC_ASSERT(pDisp != NULL);
	pDisp->AddRef();
	m_timerDispatchMap[timerID] = pDisp;
}

void CALLBACK HostTimerDispatcher::g_timer_proc_legacy(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	HWND hWnd = reinterpret_cast<HWND>(dwUser);
	SendMessage(hWnd, UWM_TIMER, uTimerID, 0);
}

void CALLBACK HostTimerDispatcher::g_timer_proc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	HWND hWnd = reinterpret_cast<HWND>(dwUser);
	SendMessage(hWnd, UWM_TIMER_NEW, uTimerID, 0);
}
